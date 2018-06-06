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

#include <include/bluzelle.hpp>
#include <crud/crud_base.hpp>
#include <include/boost_asio_beast.hpp>
#include <memory>


// minimalistic http server to answer solidity/oracle CRUD commands
// This could of been merged with node, but the websocket server will be going away eventually.

namespace bzn::http
{
    class server : public std::enable_shared_from_this<server>
    {
    public:
        server(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::crud_base> crud, const boost::asio::ip::tcp::endpoint& ep);

        void start();

    private:
        void do_accept();

        std::unique_ptr<bzn::asio::tcp_acceptor_base> tcp_acceptor;
        std::unique_ptr<bzn::asio::tcp_socket_base>   acceptor_socket;
        std::shared_ptr<bzn::asio::io_context_base>   io_context;
        std::shared_ptr<bzn::crud_base>               crud;

        std::once_flag start_once;
    };

} // bzn::http
