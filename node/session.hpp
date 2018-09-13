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
#include <memory>
#include <mutex>
#include <list>

#include <gtest/gtest_prod.h>


namespace bzn
{
    class session final : public bzn::session_base, public std::enable_shared_from_this<session>
    {
    public:
        session(std::shared_ptr<bzn::asio::io_context_base> io_context, bzn::session_id session_id, std::shared_ptr<bzn::beast::websocket_stream_base> websocket, const std::chrono::milliseconds& ws_idle_timeout);

        void start(bzn::message_handler handler) override;

        void send_message(std::shared_ptr<bzn::message> msg, bool end_session) override;

        void send_message(std::shared_ptr<std::string> msg, bool end_session) override;

        void send_datagram(std::shared_ptr<std::string> msg) override;

        void close() override;

        bzn::session_id get_session_id() override { return this->session_id; }

    private:
        FRIEND_TEST(node_session, test_that_when_message_arrives_registered_callback_is_executed);

        void do_read();

        void start_idle_timeout();

        std::unique_ptr<bzn::asio::strand_base> strand;
        const bzn::session_id session_id;

        std::shared_ptr<bzn::beast::websocket_stream_base> websocket;
        std::unique_ptr<bzn::asio::steady_timer_base> idle_timer;

        const std::chrono::milliseconds ws_idle_timeout;
        bzn::message_handler handler;

        const bool ignore_json_errors = false;

        std::mutex write_lock;
    };

} // blz
