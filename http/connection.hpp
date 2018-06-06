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

#pragma once

#include <include/boost_asio_beast.hpp>
#include <crud/crud_base.hpp>
#include <storage/storage_base.hpp>
#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <gtest/gtest_prod.h>


namespace bzn::http
{
    class connection : public std::enable_shared_from_this<connection>
    {
    public:
        connection(std::shared_ptr<bzn::asio::io_context_base> io_context, std::unique_ptr<bzn::beast::http_socket_base> http_socket, std::shared_ptr<bzn::crud_base> crud);

        void start();

    private:
        FRIEND_TEST(http_connection, test_that_http_get_calls_crud_read_and_returns_error_when_not_found);
        FRIEND_TEST(http_connection, test_that_http_get_calls_crud_read_and_returns_value_when_found);
        FRIEND_TEST(http_connection, test_that_post_calls_crud_create_and_returns_success);
        FRIEND_TEST(http_connection, test_that_post_calls_crud_create_and_returns_redirect_when_not_the_leader);
        FRIEND_TEST(http_connection, test_that_post_calls_crud_update_and_returns_success);
        FRIEND_TEST(http_connection, test_that_post_calls_crud_delete_and_returns_success);

        void handle_get(const std::vector<std::string>& path);
        void handle_post(const std::vector<std::string>& path);

        void do_read_request();
        void write_response();

        void start_deadline_timer();

        std::shared_ptr<bzn::beast::http_socket_base> http_socket;
        std::unique_ptr<bzn::asio::steady_timer_base> deadline_timer;
        std::shared_ptr<bzn::crud_base> crud;

        boost::beast::flat_buffer buffer{bzn::MAX_VALUE_SIZE + 1024}; // add a bit of room for a header
        boost::beast::http::request<boost::beast::http::dynamic_body> request;
        boost::beast::http::response<boost::beast::http::dynamic_body> response;

        std::once_flag start_once;
    };

} // bzn::http