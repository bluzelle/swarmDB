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

#include <mocks/smart_mock_io.hpp>
#include <include/bluzelle.hpp>

using namespace ::testing;
using namespace bzn;


bzn::asio::smart_mock_io::smart_mock_io()
{
    EXPECT_CALL(*this, make_unique_strand()).WillRepeatedly(Invoke(
            [&]()
            {
                auto strand = std::make_unique<bzn::asio::Mockstrand_base>();
                EXPECT_CALL(*strand, wrap(A<bzn::asio::close_handler>())).WillRepeatedly(ReturnArg<0>());
                EXPECT_CALL(*strand, wrap(A<bzn::asio::read_handler>())).WillRepeatedly(ReturnArg<0>());
                EXPECT_CALL(*strand, wrap(A<bzn::asio::task>())).WillRepeatedly(ReturnArg<0>());
                EXPECT_CALL(*strand, post(A<bzn::asio::task>())).WillRepeatedly(Invoke(
                        [&](auto task)
                        {
                            this->wrapped_post(task);
                        }));

                return strand;
            }));

    EXPECT_CALL(*this, post(_)).WillRepeatedly(Invoke(
            [&](auto func)
            {
                this->wrapped_post(func);
            }));

    EXPECT_CALL(*this, make_unique_steady_timer()).WillRepeatedly(Invoke(
            [&]()
            {
                auto id = timer_count++;

                auto timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
                EXPECT_CALL(*timer, async_wait(_)).WillRepeatedly(Invoke(
                        [&, id](auto wh)
                        {
                            timer_callbacks[id] = wh;
                        }));
                EXPECT_CALL(*timer, expires_from_now(_)).Times(AnyNumber());
                return timer;
            }));

    EXPECT_CALL(*this, make_unique_tcp_socket(_)).WillRepeatedly(Invoke(
            [&](auto& /*strand*/)
            {
                return this->make_unique_tcp_socket();
            }));

    EXPECT_CALL(*this, make_unique_tcp_socket()).WillRepeatedly(Invoke(
            [&]()
            {
                auto id = socket_count++;

                auto mock_socket = std::make_unique<bzn::asio::Mocktcp_socket_base>();

                static boost::asio::io_context io;
                static boost::asio::ip::tcp::socket socket(io);
                this->socket_id_map.insert_or_assign(socket.native_handle(), id);
                EXPECT_CALL(*mock_socket, get_tcp_socket()).WillRepeatedly(ReturnRef(socket));

                EXPECT_CALL(*mock_socket, remote_endpoint()).Times(AnyNumber());

                EXPECT_CALL(*mock_socket, async_connect(_, _)).Times(AtMost(1)).WillOnce(Invoke(
                        [&](auto, auto handler)
                        {
                            this->wrapped_post(std::bind(handler,
                                    this->tcp_connect_works ? boost::system::error_code{} : boost::asio::error::connection_refused));
                        }));

                return mock_socket;
            }));

    EXPECT_CALL(*this, make_unique_tcp_acceptor(_)).Times(AtMost(1)).WillOnce(Invoke(
            [&](auto& /*ep*/)
            {
                auto mock_acceptor = std::make_unique<bzn::asio::Mocktcp_acceptor_base>();

                EXPECT_CALL(*mock_acceptor, async_accept(_, _)).WillRepeatedly(Invoke(
                        [&](auto& socket, auto handler)
                        {
                            auto id = this->socket_id_map.at(socket.get_tcp_socket().native_handle());
                            this->tcp_accept_handlers.insert_or_assign(id, handler);
                        }));

                return mock_acceptor;
            }));

    EXPECT_CALL(*(this->websocket), make_unique_websocket_stream(_)).WillRepeatedly(Invoke(
            [&](auto& socket)
            {
                auto id = this->socket_id_map.at(socket.native_handle());
                this->socket_is_open.insert_or_assign(id, true);
                auto wss = std::make_unique<bzn::beast::Mockwebsocket_stream_base>();

                EXPECT_CALL(*wss, async_accept(_)).Times(AtMost(1)).WillOnce(Invoke(
                        [&, id](auto handler)
                        {
                            this->ws_accept_handlers.insert_or_assign(id, handler);
                        }));

                EXPECT_CALL(*wss, async_write(_, _)).WillRepeatedly(Invoke(
                        [&, id](const boost::asio::mutable_buffers_1& buffer, auto handler)
                        {
                            this->ws_write_closures.insert_or_assign(id,
                                    [handler, &buffer]()
                                    {
                                        char* raw_buf = static_cast<char*>(buffer.begin()->data());
                                        std::string result(raw_buf, buffer.begin()->size());
                                        handler(boost::system::error_code{}, result.size());

                                        return result;
                                    });
                        }));

                EXPECT_CALL(*wss, async_read(_, _)).WillRepeatedly(Invoke(
                        [&, id](auto& buffer, auto handler)
                        {
                            this->ws_read_closures.insert_or_assign(id,
                                    [handler, &buffer](std::string data)
                                    {
                                        size_t n = boost::asio::buffer_copy(buffer.prepare(data.size()), boost::asio::buffer(data));
                                        buffer.commit(n);

                                        handler(boost::system::error_code{}, data.size());
                                    });
                        }));

                EXPECT_CALL(*wss, async_handshake(_, _, _)).Times(AtMost(1)).WillRepeatedly(Invoke(
                        [&](auto, auto, auto handler)
                        {
                            this->wrapped_post(std::bind(handler, boost::system::error_code{}));
                        }));

                EXPECT_CALL(*wss, is_open()).WillRepeatedly(Invoke(
                        [&, id]()
                        {
                            return this->socket_is_open.at(id);
                        }));

                this->ws_closed.insert_or_assign(id, false);
                EXPECT_CALL(*wss, async_close(_, _)).WillRepeatedly(Invoke(
                        [&, id](auto /*reason*/, auto handler)
                        {
                            this->ws_closed.insert_or_assign(id, true);
                            this->wrapped_post(std::bind(handler, boost::system::error_code{}));
                        }));

                EXPECT_CALL(*wss, binary(_)).Times(AnyNumber());

                return wss;
            }));
}

void
bzn::asio::smart_mock_io::do_incoming_connection(size_t id)
{
    this->tcp_accept_handlers.at(id)(boost::system::error_code{});
    this->yield_until_clear();
    this->ws_accept_handlers.at(id)(boost::system::error_code{});
    this->yield_until_clear();
}

void
bzn::asio::smart_mock_io::shutdown()
{
    if (this->pending_real_callbacks > 0)
    {
        LOG(error) << "Warning: shutting down mock io context while it still has pending real callbacks to execute";
    }

    // These callbacks are likely to transitively hold a shared pointers to us,
    // so cleaning them up is necessary for us to be cleaned up
    this->timer_callbacks.clear();
    this->ws_read_closures.clear();
    this->ws_write_closures.clear();
    this->ws_accept_handlers.clear();
    this->tcp_accept_handlers.clear();
}

void
bzn::asio::smart_mock_io::wrapped_post(bzn::asio::task task)
{
    this->pending_real_callbacks++;
    boost::asio::post([this, task]()
        {
            task();
            this->pending_real_callbacks--;
        });
}

void
bzn::asio::smart_mock_io::yield_until_clear()
{
    uint64_t sleep_time = 1;
    while (this->pending_real_callbacks > 0)
    {
        if (sleep_time > 1)
        {
            LOG(debug) << "some callbacks are still pending; mock waiting for " << sleep_time << " ms";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        sleep_time *= 2;
    }
}
