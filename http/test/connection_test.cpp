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

#include <http/connection.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_crud_base.hpp>

#include <gmock/gmock.h>

using namespace ::testing;

// todo: use fixture and param tests...

namespace bzn::http
{

    TEST(http_connection, test_that_http_get_calls_crud_read_and_returns_error_when_not_found)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_http_socket = std::make_unique<bzn::beast::Mockhttp_socket_base>();
        auto mock_crud = std::make_shared<bzn::Mockcrud_base>();
        auto mock_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        EXPECT_CALL(*mock_http_socket, async_write(_,_));

        bzn::beast::read_handler rh;
        EXPECT_CALL(*mock_http_socket, async_read(_,_,_)).WillOnce(Invoke(
            [&](auto,auto,auto handler)
            {
                rh = handler;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_timer);
            }));

        auto con = std::make_shared<bzn::http::connection>(mock_io_context, std::move(mock_http_socket), mock_crud);
        con->start();

        // setup the request...
        con->request.method(boost::beast::http::verb::get);
        con->request.target("/read/uuid/key");

        EXPECT_CALL(*mock_crud, handle_read(_, _, _)).WillOnce(Invoke(
            [](auto, const database_msg& request, database_response& response)
            {
                EXPECT_EQ(request.header().db_uuid(), "uuid");
                EXPECT_EQ(request.read().key(), "key");
                response.mutable_error()->set_message("error");
            }));

        rh(boost::beast::error_code(), 0);

        // should get an error response
        std::stringstream ss;
        ss << boost::beast::buffers(con->response.body().data());
        EXPECT_EQ(ss.str(), "err");

    }


    TEST(http_connection, test_that_http_get_calls_crud_read_and_returns_value_when_found)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_http_socket = std::make_unique<bzn::beast::Mockhttp_socket_base>();
        auto mock_crud = std::make_shared<bzn::Mockcrud_base>();
        auto mock_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        EXPECT_CALL(*mock_http_socket, async_write(_,_));

        bzn::beast::read_handler rh;
        EXPECT_CALL(*mock_http_socket, async_read(_,_,_)).WillOnce(Invoke(
            [&](auto,auto,auto handler)
            {
                rh = handler;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_timer);
            }));

        auto con = std::make_shared<bzn::http::connection>(mock_io_context, std::move(mock_http_socket), mock_crud);
        con->start();

        // setup the request...
        con->request.method(boost::beast::http::verb::get);
        con->request.target("/read/uuid/key");

        // test with a response to the query...
        EXPECT_CALL(*mock_crud, handle_read(_, _, _)).WillOnce(Invoke(
            [](auto, const database_msg& request, database_response& response)
            {
                EXPECT_EQ(request.header().db_uuid(), "uuid");
                EXPECT_EQ(request.read().key(), "key");
                response.mutable_read()->set_key("key");
                response.mutable_read()->set_value("value");
            }));

        rh(boost::beast::error_code(), 0);

        // should get the value
        std::stringstream ss;
        ss << boost::beast::buffers(con->response.body().data());
        EXPECT_EQ(ss.str(), "ackvalue");
    }


    TEST(http_connection, test_that_post_calls_crud_create_and_returns_success)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_http_socket = std::make_unique<bzn::beast::Mockhttp_socket_base>();
        auto mock_crud = std::make_shared<bzn::Mockcrud_base>();
        auto mock_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        EXPECT_CALL(*mock_http_socket, async_write(_, _));

        bzn::beast::read_handler rh;
        EXPECT_CALL(*mock_http_socket, async_read(_, _, _)).WillOnce(Invoke(
            [&](auto, auto, auto handler)
            {
                rh = handler;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_timer);
            }));

        auto con = std::make_shared<bzn::http::connection>(mock_io_context, std::move(mock_http_socket), mock_crud);
        con->start();

        // setup the request...
        con->request.method(boost::beast::http::verb::post);
        con->request.target("/create/uuid/key");
        boost::beast::ostream(con->request.body()) << "value";

        // test with a response to the query...
        EXPECT_CALL(*mock_crud, handle_create(_, _, _)).WillOnce(Invoke(
            [](auto, const database_msg& request, database_response& /*response*/)
            {
                EXPECT_EQ(request.header().db_uuid(), "uuid");
                EXPECT_EQ(request.create().key(), "key");
            }));

        rh(boost::beast::error_code(), 0);

        std::stringstream ss;
        ss << boost::beast::buffers(con->response.body().data());
        EXPECT_EQ(ss.str(), "ack");
    }


    TEST(http_connection, test_that_post_calls_crud_create_and_returns_redirect_when_not_the_leader)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_http_socket = std::make_unique<bzn::beast::Mockhttp_socket_base>();
        auto mock_crud = std::make_shared<bzn::Mockcrud_base>();
        auto mock_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        EXPECT_CALL(*mock_http_socket, async_write(_, _));

        bzn::beast::read_handler rh;
        EXPECT_CALL(*mock_http_socket, async_read(_, _, _)).WillOnce(Invoke(
            [&](auto, auto, auto handler)
            {
                rh = handler;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_timer);
            }));

        auto con = std::make_shared<bzn::http::connection>(mock_io_context, std::move(mock_http_socket), mock_crud);
        con->start();

        // setup the request...
        con->request.method(boost::beast::http::verb::post);
        con->request.target("/create/uuid/key");
        boost::beast::ostream(con->request.body()) << "value";

        // test with a response to the query...
        EXPECT_CALL(*mock_crud, handle_create(_, _, _)).WillOnce(Invoke(
            [](auto, const database_msg& request, database_response& response)
            {
                EXPECT_EQ(request.header().db_uuid(), "uuid");
                EXPECT_EQ(request.create().key(), "key");

                // force a redirect...
                response.mutable_redirect()->set_leader_host("127.0.0.1");
                response.mutable_redirect()->set_leader_http_port(8888);
            }));

        rh(boost::beast::error_code(), 0);

        EXPECT_EQ(con->response.result(), boost::beast::http::status::temporary_redirect);
        EXPECT_EQ(std::string(con->response.at(boost::beast::http::field::location)), "http://127.0.0.1:8888/create/uuid/key");
    }


    TEST(http_connection, test_that_post_calls_crud_update_and_returns_success)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_http_socket = std::make_unique<bzn::beast::Mockhttp_socket_base>();
        auto mock_crud = std::make_shared<bzn::Mockcrud_base>();
        auto mock_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        EXPECT_CALL(*mock_http_socket, async_write(_, _));

        bzn::beast::read_handler rh;
        EXPECT_CALL(*mock_http_socket, async_read(_, _, _)).WillOnce(Invoke(
            [&](auto, auto, auto handler)
            {
                rh = handler;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_timer);
            }));

        auto con = std::make_shared<bzn::http::connection>(mock_io_context, std::move(mock_http_socket), mock_crud);
        con->start();

        // setup the request...
        con->request.method(boost::beast::http::verb::post);
        con->request.target("/update/uuid/key");
        boost::beast::ostream(con->request.body()) << "value";

        // test with a response to the query...
        EXPECT_CALL(*mock_crud, handle_update(_, _, _)).WillOnce(Invoke(
            [](auto, const database_msg& request, database_response& /*response*/)
            {
                EXPECT_EQ(request.header().db_uuid(), "uuid");
                EXPECT_EQ(request.update().key(), "key");
            }));

        rh(boost::beast::error_code(), 0);

        std::stringstream ss;
        ss << boost::beast::buffers(con->response.body().data());
        EXPECT_EQ(ss.str(), "ack");
    }


    TEST(http_connection, test_that_post_calls_crud_delete_and_returns_success)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_http_socket = std::make_unique<bzn::beast::Mockhttp_socket_base>();
        auto mock_crud = std::make_shared<bzn::Mockcrud_base>();
        auto mock_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        EXPECT_CALL(*mock_http_socket, async_write(_, _));

        bzn::beast::read_handler rh;
        EXPECT_CALL(*mock_http_socket, async_read(_, _, _)).WillOnce(Invoke(
            [&](auto, auto, auto handler)
            {
                rh = handler;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_timer);
            }));

        auto con = std::make_shared<bzn::http::connection>(mock_io_context, std::move(mock_http_socket), mock_crud);
        con->start();

        // setup the request...
        con->request.method(boost::beast::http::verb::post);
        con->request.target("/delete/uuid/key");
        boost::beast::ostream(con->request.body()) << "value";

        // test with a response to the query...
        EXPECT_CALL(*mock_crud, handle_delete(_, _, _)).WillOnce(Invoke(
            [](auto, const database_msg& request, database_response& /*response*/)
            {
                EXPECT_EQ(request.header().db_uuid(), "uuid");
                EXPECT_EQ(request.delete_().key(), "key");
            }));

        rh(boost::beast::error_code(), 0);

        std::stringstream ss;
        ss << boost::beast::buffers(con->response.body().data());
        EXPECT_EQ(ss.str(), "ack");
    }


    TEST(http_connection, test_that_post_calls_crud_method_and_timeout_closes_connection)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_http_socket = std::make_unique<bzn::beast::Mockhttp_socket_base>();
        auto mock_crud = std::make_shared<bzn::Mockcrud_base>();
        auto mock_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();

        bzn::beast::read_handler rh;
        EXPECT_CALL(*mock_http_socket, async_read(_, _, _)).WillOnce(Invoke(
            [&](auto, auto, auto handler)
            {
                rh = handler;
            }));

        EXPECT_CALL(*mock_timer, expires_from_now(_));

        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_timer, async_wait(_)).WillOnce(Invoke(
            [&](auto handler)
            {
                wh = handler;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_timer);
            }));

        EXPECT_CALL(*mock_http_socket, close());

        auto con = std::make_shared<bzn::http::connection>(mock_io_context, std::move(mock_http_socket), mock_crud);
        con->start();

        wh(boost::system::error_code());
    }
}
