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
#include <options/options.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <boost/asio/buffer.hpp>
#include <mocks/mock_chaos_base.hpp>
#include <mocks/smart_mock_io.hpp>
#include <mocks/mock_monitor.hpp>
#include <mocks/mock_crypto_base.hpp>
#include <functional>

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
    std::shared_ptr<bzn::asio::mock_io_context_base> io_context = std::make_shared<bzn::asio::mock_io_context_base>();
    std::shared_ptr<bzn::mock_chaos_base> mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
    std::shared_ptr<bzn::mock_monitor> monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();

    bzn::asio::wait_handler timer_expiry;

    session_test()
    {
        EXPECT_CALL(*(this->io_context), make_unique_steady_timer()).WillRepeatedly(Invoke(
                [&]()
                {
                    auto timer = std::make_unique<bzn::asio::mock_steady_timer_base>();
                    EXPECT_CALL(*timer, async_wait(_)).WillRepeatedly(Invoke(
                            [&](auto wh)
                            {
                                this->timer_expiry = wh;
                            }));
                    EXPECT_CALL(*timer, expires_from_now(_)).Times(AnyNumber());
                    return timer;
                }));

        EXPECT_CALL(*(this->io_context), make_unique_strand()).WillRepeatedly(Invoke(
                []()
                {
                    auto strand = std::make_unique<bzn::asio::mock_strand_base>();
                    EXPECT_CALL(*strand, wrap(A<bzn::asio::close_handler>())).WillRepeatedly(ReturnArg<0>());
                    EXPECT_CALL(*strand, wrap(A<bzn::asio::read_handler>())).WillRepeatedly(ReturnArg<0>());
                    EXPECT_CALL(*strand, wrap(A<bzn::asio::task>())).WillRepeatedly(ReturnArg<0>());
                    EXPECT_CALL(*strand, post(A<bzn::asio::task>())).WillRepeatedly(Invoke(
                            [](auto task)
                            {
                                task();
                            }));
                    return strand;
                }));

        EXPECT_CALL(*(this->io_context), post(_)).WillRepeatedly(Invoke(
                [](auto task)
                {
                    task();
                }));
    }
};

class session_test2 : public Test
{
public:
    std::shared_ptr<bzn::asio::smart_mock_io> mock_io = std::make_shared<bzn::asio::smart_mock_io>();
    std::shared_ptr<bzn::mock_chaos_base> mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
    std::shared_ptr<bzn::mock_monitor> monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();
    std::shared_ptr<bzn::mock_crypto_base> mock_crypto = std::make_shared<NiceMock<bzn::mock_crypto_base>>();
    std::shared_ptr<bzn::options> options = std::make_shared<bzn::options>();


    uint handler_called = 0;
    std::shared_ptr<bzn::session> session;

    session_test2()
    {
        session = std::make_shared<bzn::session>(this->mock_io, 0, TEST_ENDPOINT, this->mock_chaos, [&](auto, auto){this->handler_called++;}, TEST_TIMEOUT, std::list<bzn::session_shutdown_handler>{[](){}}, this->mock_crypto, this->monitor, this->options, std::nullopt);
    }

    void yield()
    {
        // For making sure that async callbacks get a chance to be executed before we assert their result
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ~session_test2()
    {
        this->mock_io->shutdown();
    }
};

namespace bzn
{

    TEST_F(session_test, test_that_when_session_starts_it_accepts_and_read_is_scheduled)
    {
        auto mock_websocket_stream = std::make_shared<bzn::beast::mock_websocket_stream_base>();
        EXPECT_CALL(*mock_websocket_stream, is_open()).WillRepeatedly(Return(true));

        bzn::asio::accept_handler accept_handler;
        EXPECT_CALL(*mock_websocket_stream, async_accept(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                {
                    accept_handler = handler;
                }));

        EXPECT_CALL(*mock_websocket_stream, async_read(_,_));

        auto session = std::make_shared<bzn::session>(this->io_context, bzn::session_id(1), TEST_ENDPOINT, this->mock_chaos, [](auto, auto){}, TEST_TIMEOUT, std::list<bzn::session_shutdown_handler>{[](){}}, nullptr, this->monitor, nullptr, std::nullopt);
        session->accept(mock_websocket_stream);
        accept_handler(boost::system::error_code{});

        EXPECT_EQ(bzn::session_id(1), session->get_session_id());
    }

    TEST_F(session_test2, session_sets_swarm_id_when_sending_a_message)
    {
        const bzn::uuid_t TEST_SWARM_UUID{"0096a9f1-544c-443f-9783-e54b322c1544"};
        this->options->get_mutable_simple_options().set("swarm_id", TEST_SWARM_UUID);

        auto msg{std::make_shared<bzn_envelope>()};

        this->session->send_signed_message(msg);
        this->yield();

        EXPECT_EQ(TEST_SWARM_UUID, msg->swarm_id());
    }

    TEST_F(session_test2, message_queued_before_handshake_gets_sent)
    {
        this->session->send_message(std::make_shared<std::string>("hihi"));

        this->session->accept(this->mock_io->websocket->make_unique_websocket_stream(
                                this->mock_io->make_unique_tcp_socket()->get_tcp_socket()));
        this->yield();
        this->mock_io->ws_accept_handlers.at(0)(boost::system::error_code{});

        this->yield();

        EXPECT_EQ(this->mock_io->ws_write_closures.at(0)(), "hihi");
    }

    TEST_F(session_test2, idle_timeout_closes_session)
    {
        this->session->accept(this->mock_io->websocket->make_unique_websocket_stream(
                                this->mock_io->make_unique_tcp_socket()->get_tcp_socket()));
        this->yield();
        this->mock_io->ws_accept_handlers.at(0)(boost::system::error_code{});

        this->mock_io->timer_callbacks.at(0)(boost::system::error_code{});
        this->yield();
        this->mock_io->timer_callbacks.at(0)(boost::system::error_code{});
        this->yield();

        EXPECT_TRUE(this->mock_io->ws_closed.at(0));
    }

    TEST_F(session_test2, no_idle_timeout_when_connect_rejected)
    {
        auto io2 = std::make_shared<bzn::asio::smart_mock_io>();
        io2->tcp_connect_works = false;

        auto session = std::make_shared<bzn::session>(io2, 0, TEST_ENDPOINT, this->mock_chaos, [](auto, auto){}, TEST_TIMEOUT, std::list<bzn::session_shutdown_handler>{[](){}}, nullptr, this->monitor, nullptr, std::nullopt);
        session->open(io2->websocket, nullptr);

        this->yield();

        EXPECT_EQ(io2->timer_callbacks.size(), 0u);
        io2->shutdown();
    }

    TEST_F(session_test2, additional_shutdown_handlers_can_be_added_to_session)
    {
        auto io2 = std::make_shared<bzn::asio::smart_mock_io>();

        std::vector<uint8_t> handler_counters { 0,0,0 };
        {
            auto session = std::make_shared<bzn::session>(io2
                    , 0, TEST_ENDPOINT, this->mock_chaos, [](auto, auto){}, TEST_TIMEOUT
                    , std::list<bzn::session_shutdown_handler>{[&handler_counters]() {
                        ++handler_counters[0];
                    }}, nullptr, this->monitor, nullptr, std::nullopt);

            session->add_shutdown_handler([&handler_counters](){++handler_counters[1];});
            session->add_shutdown_handler([&handler_counters](){++handler_counters[2];});

            session->open(io2->websocket, nullptr);

            io2->yield_until_clear();

            // we are just testing that this doesn't cause a segfault
            io2->timer_callbacks.at(0)(boost::system::error_code{});
            io2->timer_callbacks.at(0)(boost::system::error_code{});

        }

        io2->yield_until_clear();

        // each shutdown handler must be called exactly once.
        for(const auto handler_counter : handler_counters)
        {
            EXPECT_EQ(handler_counter, 1);
        }
        io2->shutdown();
    }

} // bzn
