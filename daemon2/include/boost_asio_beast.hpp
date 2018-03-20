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
#include <boost/asio.hpp>
#include <boost/beast.hpp>

// todo: this file needs a better name

namespace bzn::asio
{
    // types...
    using  accept_handler = std::function<void(const boost::system::error_code& ec)>;
    using    read_handler = std::function<void(const boost::system::error_code& ec, size_t bytes_transfered)>;
    using   write_handler = std::function<void(const boost::system::error_code& ec, size_t bytes_transfered)>;
    using connect_handler = std::function<void(const boost::system::error_code& ec)>;

    ///////////////////////////////////////////////////////////////////////////
    // mockable interfaces...

    class tcp_socket_base
    {
    public:
        virtual ~tcp_socket_base() = default;

        virtual void async_connect(const boost::asio::ip::tcp::endpoint& ep, bzn::asio::connect_handler handler) = 0;

        virtual boost::asio::ip::tcp::endpoint remote_endpoint() = 0;

        virtual boost::asio::ip::tcp::socket& get_tcp_socket() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class tcp_acceptor_base
    {
    public:
        virtual ~tcp_acceptor_base() = default;

        virtual void async_accept(bzn::asio::tcp_socket_base& socket, bzn::asio::accept_handler handler) = 0;

        virtual boost::asio::ip::tcp::acceptor& get_tcp_acceptor() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class io_context_base
    {
    public:
        virtual ~io_context_base() = default;

        virtual std::unique_ptr<bzn::asio::tcp_acceptor_base> make_unique_tcp_acceptor(const boost::asio::ip::tcp::endpoint& ep) = 0;

        virtual std::unique_ptr<bzn::asio::tcp_socket_base> make_unique_tcp_socket() = 0;

        virtual boost::asio::io_context::count_type run() = 0;

        virtual void stop() = 0;

        virtual boost::asio::io_context& get_io_context() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    // the real thing...

    class tcp_socket final : public tcp_socket_base
    {
    public:
        tcp_socket(boost::asio::io_context& io_context)
            : socket(io_context)
        {
        }

        void async_connect(const boost::asio::ip::tcp::endpoint& ep, bzn::asio::connect_handler handler)
        {
            this->socket.async_connect(ep, handler);
        }

        boost::asio::ip::tcp::endpoint remote_endpoint()
        {
            return this->socket.remote_endpoint();
        }

        boost::asio::ip::tcp::socket& get_tcp_socket()
        {
            return this->socket;
        }

    private:
        boost::asio::ip::tcp::socket socket;
    };

    ///////////////////////////////////////////////////////////////////////////

    class tcp_acceptor final : public tcp_acceptor_base
    {
    public:
        tcp_acceptor(boost::asio::io_context& io_context, const boost::asio::ip::tcp::endpoint& ep)
            : acceptor(io_context, ep)
        {
        }

        void async_accept(bzn::asio::tcp_socket_base& socket, bzn::asio::accept_handler handler)
        {
            this->acceptor.async_accept(socket.get_tcp_socket(), std::move(handler));
        }

        boost::asio::ip::tcp::acceptor& get_tcp_acceptor()
        {
            return this->acceptor;
        }

    private:
        boost::asio::ip::tcp::acceptor acceptor;
    };

    ///////////////////////////////////////////////////////////////////////////

    class io_context final : public io_context_base
    {
    public:
        std::unique_ptr<bzn::asio::tcp_acceptor_base> make_unique_tcp_acceptor(const boost::asio::ip::tcp::endpoint& ep)
        {
            return std::make_unique<bzn::asio::tcp_acceptor>(this->io_context, ep);
        }

        virtual std::unique_ptr<bzn::asio::tcp_socket_base> make_unique_tcp_socket()
        {
            return std::make_unique<bzn::asio::tcp_socket>(this->io_context);
        }

        boost::asio::io_context::count_type run()
        {
            return this->io_context.run();
        }

        void stop()
        {
            this->io_context.stop();
        }

        boost::asio::io_context& get_io_context()
        {
            return this->io_context;
        }

    private:
        boost::asio::io_context io_context;
    };

} // bzn::asio


namespace bzn::beast
{
    // types...
    using handshake_handler = std::function<void(const boost::system::error_code& ec)>;
    using close_handler = std::function<void(const boost::system::error_code& ec)>;

    ///////////////////////////////////////////////////////////////////////////
    // mockable interfaces...

    class websocket_stream_base
    {
    public:
        virtual ~websocket_stream_base() = default;

        virtual boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& get_websocket() = 0;

        virtual void async_accept(bzn::asio::accept_handler handler) = 0;

        virtual void async_read(boost::beast::multi_buffer& buffer, bzn::asio::read_handler handler) = 0;

        virtual void async_write(const boost::asio::const_buffers_1& buffer, bzn::asio::write_handler handler) = 0;

        virtual void async_close(boost::beast::websocket::close_code reason, bzn::beast::close_handler handler) = 0;

        virtual void async_handshake(const std::string& host, const std::string& target, bzn::beast::handshake_handler handler) = 0;

        virtual bool is_open() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class websocket_stream final : public websocket_stream_base
    {
    public:
        websocket_stream(boost::asio::ip::tcp::socket socket)
            : websocket(std::move(socket))
        {
        }

        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& get_websocket()
        {
            return this->websocket;
        }

        void async_accept(bzn::asio::accept_handler handler)
        {
            this->websocket.async_accept(handler);
        }

        void async_read(boost::beast::multi_buffer& buffer, bzn::asio::read_handler handler)
        {
            this->websocket.async_read(buffer, handler);
        }

        void async_write(const boost::asio::const_buffers_1& buffer, bzn::asio::write_handler handler)
        {
            this->websocket.async_write(buffer, handler);
        }

        void async_close(boost::beast::websocket::close_code reason, bzn::beast::close_handler handler)
        {
            this->websocket.async_close(reason, handler);
        }

        void async_handshake(const std::string& host, const std::string& target, bzn::beast::handshake_handler handler)
        {
            this->websocket.async_handshake(host, target, handler);
        }

        bool is_open()
        {
            return this->websocket.is_open();
        }

    private:
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> websocket;
    };

    ///////////////////////////////////////////////////////////////////////////

    class websocket_base
    {
    public:
        virtual ~websocket_base() = default;

        virtual std::unique_ptr<bzn::beast::websocket_stream_base> make_unique_websocket_stream(boost::asio::ip::tcp::socket& socket) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class websocket final : public websocket_base
    {
    public:
        std::unique_ptr<bzn::beast::websocket_stream_base> make_unique_websocket_stream(boost::asio::ip::tcp::socket& socket)
        {
            return std::make_unique<bzn::beast::websocket_stream>(std::move(socket));
        }
    };

} // bzn::beast
