// Copyright (C) 2019 Bluzelle
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

#pragma once

#include <mocks/mock_boost_asio_beast.hpp>

namespace bzn::asio
{
    class smart_mock_io : public Mockio_context_base
    {
    public:

        smart_mock_io();
        void do_incoming_connection(size_t id);
        void shutdown();

        std::shared_ptr<bzn::asio::io_context_base> real_io_context = std::make_shared<bzn::asio::io_context>();
        std::shared_ptr<bzn::beast::Mockwebsocket_base> websocket = std::make_shared<bzn::beast::Mockwebsocket_base>();

        size_t timer_count = 0;
        std::map<size_t, bzn::asio::wait_handler> timer_callbacks;

        size_t socket_count = 0;
        std::map<size_t, bzn::asio::accept_handler> tcp_accept_handlers;
        std::map<size_t, bzn::asio::accept_handler> ws_accept_handlers;

        std::map<size_t, std::function<std::string()>> ws_write_closures;
        std::map<size_t, std::function<void(std::string)>> ws_read_closures;

        std::map<size_t, bool> ws_closed;

        std::map<size_t, bool> socket_is_open;

        std::map<boost::asio::ip::tcp::socket::native_handle_type, size_t> socket_id_map;

        bool tcp_connect_works = true;

    };
}
