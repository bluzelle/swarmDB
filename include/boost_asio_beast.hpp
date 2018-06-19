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

#include <boost/asio.hpp>
#include <boost/beast.hpp>

// todo: this file needs a better name!

namespace bzn::asio
{
    // types...
    using  accept_handler = std::function<void(const boost::system::error_code& ec)>;
    using    read_handler = std::function<void(const boost::system::error_code& ec, size_t bytes_transfered)>;
    using   write_handler = std::function<void(const boost::system::error_code& ec, size_t bytes_transfered)>;
    using connect_handler = std::function<void(const boost::system::error_code& ec)>;
    using   close_handler = std::function<void(const boost::system::error_code& ec)>;
    using    wait_handler = std::function<void(const boost::system::error_code& ec)>;

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

    class steady_timer_base
    {
    public:
        virtual ~steady_timer_base() = default;

        virtual void async_wait(wait_handler handler) = 0;

        virtual std::size_t expires_from_now(const std::chrono::milliseconds& expiry_time) = 0;

        virtual void cancel() = 0;

        virtual boost::asio::steady_timer& get_steady_timer() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class strand_base
    {
    public:
        virtual ~strand_base() = default;

        virtual bzn::asio::write_handler wrap(write_handler handler) = 0;

        virtual bzn::asio::close_handler wrap(close_handler handler) = 0;

        virtual boost::asio::io_context::strand& get_strand() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class io_context_base
    {
    public:
        virtual ~io_context_base() = default;

        virtual std::unique_ptr<bzn::asio::tcp_acceptor_base> make_unique_tcp_acceptor(const boost::asio::ip::tcp::endpoint& ep) = 0;

        virtual std::unique_ptr<bzn::asio::tcp_socket_base> make_unique_tcp_socket() = 0;

        virtual std::unique_ptr<bzn::asio::steady_timer_base> make_unique_steady_timer() = 0;

        virtual std::unique_ptr<bzn::asio::strand_base> make_unique_strand() = 0;

        virtual boost::asio::io_context::count_type run() = 0;

        virtual void stop() = 0;

        virtual boost::asio::io_context& get_io_context() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    // the real thing...

    class tcp_socket final : public tcp_socket_base
    {
    public:
        explicit tcp_socket(boost::asio::io_context& io_context)
            : socket(io_context)
        {
        }

        void async_connect(const boost::asio::ip::tcp::endpoint& ep, bzn::asio::connect_handler handler) override
        {
            this->socket.async_connect(ep, handler);
        }

        boost::asio::ip::tcp::endpoint remote_endpoint() override
        {
            return this->socket.remote_endpoint();
        }

        boost::asio::ip::tcp::socket& get_tcp_socket() override
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
        explicit tcp_acceptor(boost::asio::io_context& io_context, const boost::asio::ip::tcp::endpoint& ep)
            : acceptor(io_context, ep)
        {
        }

        void async_accept(bzn::asio::tcp_socket_base& socket, bzn::asio::accept_handler handler) override
        {
            this->acceptor.async_accept(socket.get_tcp_socket(), std::move(handler));
        }

        boost::asio::ip::tcp::acceptor& get_tcp_acceptor() override
        {
            return this->acceptor;
        }

    private:
        boost::asio::ip::tcp::acceptor acceptor;
    };

    ///////////////////////////////////////////////////////////////////////////

    class steady_timer final : public steady_timer_base
    {
    public:
        explicit steady_timer(boost::asio::io_context& io_context)
            : timer(io_context)
        {
        }

        void async_wait(wait_handler handler) override
        {
            this->timer.async_wait(handler);
        }

        std::size_t expires_from_now(const std::chrono::milliseconds& expiry_time) override
        {
            return this->timer.expires_from_now(expiry_time);
        }

        void cancel() override
        {
            this->timer.cancel();
        }

        boost::asio::steady_timer& get_steady_timer() override
        {
            return this->timer;
        }

    private:
        boost::asio::steady_timer timer;
    };

    ///////////////////////////////////////////////////////////////////////////

    class strand final : public strand_base
    {
    public:
        explicit strand(boost::asio::io_context& io_context)
            : s(io_context)
        {
        }

        bzn::asio::write_handler wrap(write_handler handler) override
        {
            return this->s.wrap(std::move(handler));
        }

        bzn::asio::close_handler wrap(close_handler handler) override
        {
            return this->s.wrap(std::move(handler));
        }

        boost::asio::io_context::strand& get_strand() override
        {
            return this->s;
        }

    private:
        boost::asio::io_context::strand s;
    };

    ///////////////////////////////////////////////////////////////////////////

    class io_context final : public io_context_base
    {
    public:
        std::unique_ptr<bzn::asio::tcp_acceptor_base> make_unique_tcp_acceptor(const boost::asio::ip::tcp::endpoint& ep) override
        {
            return std::make_unique<bzn::asio::tcp_acceptor>(this->io_context, ep);
        }

        std::unique_ptr<bzn::asio::tcp_socket_base> make_unique_tcp_socket() override
        {
            return std::make_unique<bzn::asio::tcp_socket>(this->io_context);
        }

        std::unique_ptr<bzn::asio::steady_timer_base> make_unique_steady_timer() override
        {
            return std::make_unique<bzn::asio::steady_timer>(this->io_context);
        }

        std::unique_ptr<bzn::asio::strand_base> make_unique_strand() override
        {
            return std::make_unique<bzn::asio::strand>(this->io_context);
        }

        boost::asio::io_context::count_type run() override
        {
            return this->io_context.run();
        }

        void stop() override
        {
            this->io_context.stop();
        }

        boost::asio::io_context& get_io_context() override
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
    using read_handler  = std::function<void(const boost::beast::error_code& ec, std::size_t bytes_transferred)>;
    using write_handler = std::function<void(const boost::beast::error_code& ec, std::size_t bytes_transferred)>;
    using close_handler = std::function<void(const boost::system::error_code& ec)>;

    ///////////////////////////////////////////////////////////////////////////
    // mockable interfaces...

    class http_socket_base
    {
    public:
        virtual ~http_socket_base() = default;

        virtual boost::asio::ip::tcp::socket& get_socket() = 0;

        virtual void async_read(boost::beast::flat_buffer& buffer, boost::beast::http::request<boost::beast::http::dynamic_body>& request, bzn::beast::read_handler handler) = 0;

        virtual void async_write(boost::beast::http::response<boost::beast::http::dynamic_body>& response, bzn::beast::write_handler handler) = 0;

        virtual void close() = 0;
    };

    class http_socket final : public http_socket_base
    {
    public:
        explicit http_socket(boost::asio::ip::tcp::socket socket)
            : socket(std::move(socket))
        {
        }

        boost::asio::ip::tcp::socket& get_socket() override
        {
            return this->socket;
        }

        void async_read(boost::beast::flat_buffer& buffer, boost::beast::http::request<boost::beast::http::dynamic_body>& request, bzn::beast::read_handler handler) override
        {
            boost::beast::http::async_read(this->socket, buffer, request, std::move(handler));
        }

        void async_write(boost::beast::http::response<boost::beast::http::dynamic_body>& response, bzn::beast::write_handler handler) override
        {
            boost::beast::http::async_write(this->socket, response, std::move(handler));
        }

        void close() override
        {
            this->socket.close();
        }

    private:
        boost::asio::ip::tcp::socket socket;
    };

    ///////////////////////////////////////////////////////////////////////////

    class websocket_stream_base
    {
    public:
        virtual ~websocket_stream_base() = default;

        virtual boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& get_websocket() = 0;

        virtual void async_accept(bzn::asio::accept_handler handler) = 0;

        virtual void async_read(boost::beast::multi_buffer& buffer, bzn::asio::read_handler handler) = 0;

        virtual void async_write(const boost::asio::mutable_buffers_1& buffer, bzn::asio::write_handler handler) = 0;

        virtual void async_close(boost::beast::websocket::close_code reason, bzn::beast::close_handler handler) = 0;

        virtual void async_handshake(const std::string& host, const std::string& target, bzn::beast::handshake_handler handler) = 0;

        virtual bool is_open() = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class websocket_stream final : public websocket_stream_base
    {
    public:
        explicit websocket_stream(boost::asio::ip::tcp::socket socket)
            : websocket(std::move(socket))
        {
        }

        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& get_websocket() override
        {
            return this->websocket;
        }

        void async_accept(bzn::asio::accept_handler handler) override
        {
            this->websocket.async_accept(handler);
        }

        void async_read(boost::beast::multi_buffer& buffer, bzn::asio::read_handler handler) override
        {
            this->websocket.async_read(buffer, handler);
        }

        void async_write(const boost::asio::mutable_buffers_1& buffer, bzn::asio::write_handler handler) override
        {
            this->websocket.async_write(buffer, handler);
        }

        void async_close(boost::beast::websocket::close_code reason, bzn::beast::close_handler handler) override
        {
            this->websocket.async_close(reason, handler);
        }

        void async_handshake(const std::string& host, const std::string& target, bzn::beast::handshake_handler handler) override
        {
            this->websocket.async_handshake(host, target, handler);
        }

        bool is_open() override
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
        std::unique_ptr<bzn::beast::websocket_stream_base> make_unique_websocket_stream(boost::asio::ip::tcp::socket& socket) override
        {
            return std::make_unique<bzn::beast::websocket_stream>(std::move(socket));
        }
    };

} // bzn::beast
