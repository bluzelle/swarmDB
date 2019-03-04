// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License, version 3,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <node/session.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <boost/asio/buffer.hpp>
#include <mocks/mock_chaos_base.hpp>
#include <node/test/node_test_common.hpp>

#include <gmock/gmock.h>
#include <proto/bluzelle.pb.h>

#include <list>

using namespace ::testing;

namespace
{
    const auto TEST_ENDPOINT =
            boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string("127.0.0.1"), 0}; // any port
    const auto TEST_TIMEOUT = std::chrono::milliseconds(10000);

}

class session_test : public Test
{
public:
    std::shared_ptr<bzn::asio::Mockio_context_base> io_context = std::make_shared<bzn::asio::Mockio_context_base>();
    std::shared_ptr<bzn::mock_chaos_base> mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();

    bzn::asio::wait_handler timer_expiry;

    session_test()
    {
        EXPECT_CALL(*(this->io_context), make_unique_steady_timer()).WillRepeatedly(Invoke(
                [&]()
                {
                    auto timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
                    EXPECT_CALL(*timer, async_wait(_)).WillRepeatedly(Invoke(
                            [&](auto wh)
                            {
                                this->timer_expiry = wh;
                            }));
                    EXPECT_CALL(*timer, expires_from_now(_)).Times(AnyNumber());
                    return timer;
                }));
    }
};

class session_test2 : public Test
{
public:
    bzn::smart_mock_io mock;
    std::shared_ptr<bzn::mock_chaos_base> mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
    uint handler_called = 0;
    std::shared_ptr<bzn::session> session;

    session_test2()
    {
        session = std::make_shared<bzn::session>(mock.io_context, 0, TEST_ENDPOINT, this->mock_chaos, [&](auto, auto){this->handler_called++;}, TEST_TIMEOUT, std::list<bzn::session_shutdown_handler>{[](){}}, nullptr);
    }

    void yield()
    {
        // For making sure that async callbacks get a chance to be executed before we assert their result
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

namespace bzn
{

    TEST_F(session_test, test_that_when_session_starts_it_accepts_and_read_is_scheduled)
    {
        auto mock_websocket_stream = std::make_shared<bzn::beast::Mockwebsocket_stream_base>();
        EXPECT_CALL(*mock_websocket_stream, is_open()).WillRepeatedly(Return(true));

        bzn::asio::accept_handler accept_handler;
        EXPECT_CALL(*mock_websocket_stream, async_accept(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                {
                    accept_handler = handler;
                }));

        EXPECT_CALL(*mock_websocket_stream, async_read(_,_));

        auto session = std::make_shared<bzn::session>(this->io_context, bzn::session_id(1), TEST_ENDPOINT, this->mock_chaos, [](auto, auto){}, TEST_TIMEOUT, std::list<bzn::session_shutdown_handler>{[](){}}, nullptr);
        session->accept(mock_websocket_stream);
        accept_handler(boost::system::error_code{});

        EXPECT_EQ(bzn::session_id(1), session->get_session_id());
    }

    TEST_F(session_test2, message_queued_before_handshake_gets_sent)
    {
        this->session->send_message(std::make_shared<std::string>("hihi"));

        this->session
            ->accept(
                    this->mock
                        .websocket
                        ->make_unique_websocket_stream(
                                this->mock
                                    .io_context
                                    ->make_unique_tcp_socket()
                                    ->get_tcp_socket()));
        this->mock.ws_accept_handlers.at(0)(boost::system::error_code{});

        this->yield();

        EXPECT_EQ(this->mock.ws_write_closures.at(0)(), "hihi");
    }

    TEST_F(session_test2, idle_timeout_closes_session)
    {
        this->session
            ->accept(
                    this->mock
                        .websocket
                        ->make_unique_websocket_stream(
                                this->mock
                                    .io_context
                                    ->make_unique_tcp_socket()
                                    ->get_tcp_socket()));
        this->mock.ws_accept_handlers.at(0)(boost::system::error_code{});

        this->mock.timer_callbacks.at(0)(boost::system::error_code{});
        this->yield();
        this->mock.timer_callbacks.at(0)(boost::system::error_code{});
        this->yield();

        EXPECT_TRUE(this->mock.ws_closed.at(0));
    }

    TEST_F(session_test2, idle_timeout_after_connect_rejected)
    {
        bzn::smart_mock_io mock;
        mock.tcp_connect_works = false;

        auto session = std::make_shared<bzn::session>(mock.io_context, 0, TEST_ENDPOINT, this->mock_chaos, [](auto, auto){}, TEST_TIMEOUT, std::list<bzn::session_shutdown_handler>{[](){}}, nullptr);
        session->open(mock.websocket);

        this->yield();

        // we are just testing that this doesn't cause a segfault
        mock.timer_callbacks.at(0)(boost::system::error_code{});
        mock.timer_callbacks.at(0)(boost::system::error_code{});
    }

    TEST_F(session_test2, additional_shutdown_handlers_can_be_added_to_session)
    {

        bzn::smart_mock_io mock;
        mock.tcp_connect_works = false;

        std::vector<uint8_t> handler_counters { 0,0,0 };
        {
            auto session = std::make_shared<bzn::session>(mock.io_context
                    , 0, TEST_ENDPOINT, this->mock_chaos, [](auto, auto){}, TEST_TIMEOUT
                    , std::list<bzn::session_shutdown_handler>{[&handler_counters]() {
                        ++handler_counters[0];
                    }}, nullptr);

            session->add_shutdown_handler([&handler_counters](){++handler_counters[1];});
            session->add_shutdown_handler([&handler_counters](){++handler_counters[2];});

            session->open(mock.websocket);

            this->yield();

            // we are just testing that this doesn't cause a segfault
            mock.timer_callbacks.at(0)(boost::system::error_code{});
            mock.timer_callbacks.at(0)(boost::system::error_code{});

        }
        this->yield();

        // each shutdown handler must be called exactly once.
        for(const auto handler_counter : handler_counters)
        {
            EXPECT_EQ(handler_counter, 1);
        }
    }

} // bzn
