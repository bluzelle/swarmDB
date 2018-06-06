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
#include <mocks/mock_boost_asio_beast.hpp>

#include <gmock/gmock.h>

using namespace ::testing;

namespace bzn::http
{
    TEST(server, test_that_server_starts_accepting_connection_and_instantiates_a_new_connection)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_tcp_acceptor = std::make_unique<bzn::asio::Mocktcp_acceptor_base>();
        auto mock_tcp_socket = std::make_unique<bzn::asio::Mocktcp_socket_base>();
        auto ep = boost::asio::ip::tcp::endpoint{boost::asio::ip::address::from_string("127.0.0.1"),8080};
        auto mock_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        bzn::asio::accept_handler ah;
        EXPECT_CALL(*mock_tcp_acceptor, async_accept(_,_)).WillRepeatedly(Invoke(
            [&](auto&, auto handler)
            {
                ah = handler;
            }));

        EXPECT_CALL(*mock_tcp_socket, remote_endpoint()).WillOnce(Return(ep));

        boost::asio::io_context io;
        boost::asio::ip::tcp::socket socket(io);
        EXPECT_CALL(*mock_tcp_socket, get_tcp_socket()).WillRepeatedly(ReturnRef(socket));

        EXPECT_CALL(*mock_io_context, make_unique_tcp_acceptor(ep)).WillOnce(Invoke(
            [&](auto )
            {
                return std::move(mock_tcp_acceptor);
            }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_timer);
            }));

        auto server = std::make_shared<bzn::http::server>(mock_io_context, nullptr, ep);

        EXPECT_CALL(*mock_io_context, make_unique_tcp_socket()).Times(2).WillRepeatedly(Invoke(
            [&]()
            {
                return std::move(mock_tcp_socket);
            }));

        server->start();

        ah(boost::system::error_code());
    }
}
