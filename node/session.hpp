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
#include <node/node_base.hpp>
#include <node/session_base.hpp>
#include <options/options_base.hpp>
#include <chaos/chaos.hpp>
#include <memory>
#include <mutex>
#include <list>
#include <atomic>
#include <node/node.hpp>

#include <gtest/gtest_prod.h>


namespace bzn
{
    class session final : public bzn::session_base, public std::enable_shared_from_this<session>
    {
    public:
        session(
                std::shared_ptr<bzn::asio::io_context_base> io_context,
                bzn::session_id session_id,
                boost::asio::ip::tcp::endpoint ep,
                std::shared_ptr<bzn::chaos_base> chaos,
                bzn::protobuf_handler proto_handler,
                std::chrono::milliseconds ws_idle_timeout,
                std::list<bzn::session_shutdown_handler> shutdown_handlers,
                std::shared_ptr<bzn::crypto_base> crypto);

        ~session();

        void send_message(std::shared_ptr<bzn::encoded_message> msg) override;
        void send_signed_message(std::shared_ptr<bzn_envelope> msg) override;

        void close() override;

        bzn::session_id get_session_id() override { return this->session_id; }

        bool is_open() const override;

        void open(std::shared_ptr<bzn::beast::websocket_base> ws_factory) override;
        void accept(std::shared_ptr<bzn::beast::websocket_stream_base> ws) override;

        void add_shutdown_handler(const bzn::session_shutdown_handler handler) override;

    private:
        void do_read();
        void do_write();

        void start_idle_timeout();

        const bzn::session_id session_id;
        const boost::asio::ip::tcp::endpoint ep;

        std::shared_ptr<bzn::asio::io_context_base> io_context;
        std::shared_ptr<bzn::beast::websocket_stream_base> websocket;
        std::shared_ptr<bzn::chaos_base> chaos;

        std::list<std::shared_ptr<bzn::encoded_message>> write_queue;

        bzn::protobuf_handler proto_handler;
        std::list<bzn::session_shutdown_handler> shutdown_handlers;

        std::unique_ptr<bzn::asio::steady_timer_base> idle_timer;
        const std::chrono::milliseconds ws_idle_timeout;

        std::mutex socket_lock;

        std::atomic<bool> writing = false;
        std::atomic<bool> reading = false;
        std::atomic<bool> closing = false;
        std::atomic<bool> activity = false;

        boost::asio::mutable_buffers_1 write_buffer;

        std::shared_ptr<bzn::crypto_base> crypto;
    };

} // blz
