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

#include <http/server.hpp>
#include <http/connection.hpp>

using namespace bzn::http;


server::server(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::crud_base> crud, const boost::asio::ip::tcp::endpoint& ep)
    : tcp_acceptor(io_context->make_unique_tcp_acceptor(ep))
    , io_context(std::move(io_context))
    , crud(std::move(crud))
{
}


void
server::start()
{
    std::call_once(this->start_once, &server::do_accept, this);
}


void
server::do_accept()
{
    this->acceptor_socket = this->io_context->make_unique_tcp_socket();

    this->tcp_acceptor->async_accept(*this->acceptor_socket,
        [self = shared_from_this()](const boost::system::error_code& ec)
        {
            if (ec)
            {
                LOG(error) << "accept failed: " << ec.message();
            }
            else
            {
                auto ep = self->acceptor_socket->remote_endpoint();

                LOG(debug) << "connection from: " << ep.address() << ":" << ep.port();

                auto hs = std::make_unique<bzn::beast::http_socket>(std::move(self->acceptor_socket->get_tcp_socket()));

                std::make_shared<bzn::http::connection>(self->io_context, std::move(hs), self->crud)->start();
            }

            self->do_accept();
        });
}
