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

#include <gmock/gmock.h>

using namespace ::testing;

namespace bzn
{
    TEST(node_session, test_that_when_session_starts_it_accepts_and_read_is_scheduled)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_websocket_stream = std::make_shared<bzn::beast::Mockwebsocket_stream_base>();
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
        auto mock_strand = std::make_unique<bzn::asio::Mockstrand_base>();

        EXPECT_CALL(*mock_io_context, make_unique_strand()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_strand);
            }));

        EXPECT_CALL(*mock_strand, wrap(An<bzn::asio::close_handler>())).WillRepeatedly(Invoke(
            [&](bzn::asio::close_handler handler)
            {
                return handler;
            }));

        EXPECT_CALL(*mock_strand, wrap(An<bzn::asio::read_handler>())).WillRepeatedly(Invoke(
            [&](bzn::asio::read_handler handler)
            {
                return handler;
            }));

        EXPECT_CALL(*mock_steady_timer, expires_from_now(std::chrono::milliseconds(1000)));

        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillOnce(Invoke(
            [&](auto handler)
            {
                wh = handler;
            }));

        EXPECT_CALL(*mock_steady_timer, cancel());

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_steady_timer);
            }));

        auto session = std::make_shared<bzn::session>(mock_io_context, mock_websocket_stream, std::chrono::milliseconds(1000));

        bzn::asio::accept_handler accept_handler;
        EXPECT_CALL(*mock_websocket_stream, async_accept(_)).WillRepeatedly(Invoke(
            [&](auto handler)
            {
                accept_handler = handler;
            }
        ));

        // only one call on success...
        EXPECT_CALL(*mock_websocket_stream, is_open()).WillOnce(Return(false));
        EXPECT_CALL(*mock_websocket_stream, async_read(_,_));

        session->start([](auto&,auto){});

        // call handler with no error and read will be scheduled...
        accept_handler(boost::system::error_code());

        // call with an error and no read will be scheduled...
        accept_handler(boost::asio::error::operation_aborted);

        // expire idle timer (will close connection)
#if 0
        EXPECT_CALL(*mock_websocket_stream, async_close(_,_));
#endif
        wh(boost::system::error_code());
    }


    TEST(node_session, test_that_when_message_arrives_registered_callback_is_executed)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto websocket_stream = std::make_shared<NiceMock<bzn::beast::Mockwebsocket_stream_base>>();
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
        auto mock_strand = std::make_unique<bzn::asio::Mockstrand_base>();

        EXPECT_CALL(*mock_io_context, make_unique_strand()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_strand);
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_steady_timer);
            }));

        EXPECT_CALL(*mock_strand, wrap(An<bzn::asio::read_handler>())).WillRepeatedly(Invoke(
            [&](bzn::asio::read_handler handler)
            {
                return handler;
            }));

        auto session = std::make_shared<bzn::session>(mock_io_context, websocket_stream, std::chrono::milliseconds(0));

        bzn::asio::accept_handler accept_handler;
        EXPECT_CALL(*websocket_stream, async_accept(_)).WillRepeatedly(Invoke(
            [&](auto handler)
            {
                accept_handler = handler;
            }));

        bzn::asio::read_handler read_handler;
        EXPECT_CALL(*websocket_stream, async_read(_,_)).WillRepeatedly(Invoke(
            [&](auto& /*buffer*/, auto handler)
            {
                // sadly we can't modify the buffer

                read_handler = handler;
            }));

        bool handler_called = false;
        session->start([&](auto&,auto){handler_called = true;});

        // yes, this is a hack!
        const_cast<bool&>(session->ignore_json_errors) = true;

        // call accept and read handler with no error and read will be scheduled...
        accept_handler(boost::system::error_code());
        read_handler(boost::system::error_code(), 0);

        // there is no way for us to modify the buffer passed to the read handler...until I figure a way!
        ASSERT_TRUE(handler_called);

        // reset flag and handler should not be called
        const_cast<bool&>(session->ignore_json_errors) = false;

        handler_called = false;
        read_handler(boost::system::error_code(), 0);
        ASSERT_FALSE(handler_called);

        // calling with an error should not do anything...
        handler_called = false;
        read_handler(boost::asio::error::operation_aborted, 0);
        ASSERT_FALSE(handler_called);
    }


    TEST(node_session, test_that_response_can_be_sent)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_strand = std::make_unique<bzn::asio::Mockstrand_base>();
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        EXPECT_CALL(*mock_strand, wrap(An<bzn::asio::write_handler>())).WillRepeatedly(Invoke(
            [&](bzn::asio::write_handler handler)
            {
                return handler;
            }));

        EXPECT_CALL(*mock_strand, wrap(An<bzn::asio::close_handler>())).WillRepeatedly(Invoke(
            [&](bzn::asio::close_handler handler)
            {
                return handler;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_strand()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_strand);
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_steady_timer);
            }));

        auto mock_websocket_stream = std::make_shared<bzn::beast::Mockwebsocket_stream_base>();
        auto session = std::make_shared<bzn::session>(mock_io_context, mock_websocket_stream, std::chrono::milliseconds(0));

        EXPECT_CALL(*mock_websocket_stream, async_write(_,_));
        EXPECT_CALL(*mock_websocket_stream, is_open()).WillOnce(Return(true));

        // expect a call to binary!
        boost::asio::io_context io;
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> socket(io);
        EXPECT_CALL(*mock_websocket_stream, get_websocket()).WillRepeatedly(ReturnRef(socket));

        // no read exepected...
        session->send_message(std::make_shared<bzn::message>("asdf"), true);

        // read should be setup...
        bzn::asio::write_handler write_handler;
        EXPECT_CALL(*mock_websocket_stream, async_write(_,_)).WillOnce(Invoke(
            [&](auto& /*buffer*/, auto handler)
            {
                write_handler = handler;
            }));

        EXPECT_CALL(*mock_websocket_stream, async_read(_,_));
        session->send_message(std::make_shared<bzn::message>("asdf"), false);
        write_handler(boost::system::error_code(), 0);

        // error no read should be setup...
        EXPECT_CALL(*mock_websocket_stream, async_close(_,_));
        write_handler(boost::asio::error::operation_aborted, 0);
    }

} // bzn
