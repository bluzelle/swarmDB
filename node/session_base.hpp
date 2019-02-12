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
#include <proto/bluzelle.pb.h>
#include <include/boost_asio_beast.hpp>


namespace bzn
{
    // forward declare...
    class session_base;

    using message_handler = std::function<void(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session)>;
    using protobuf_handler = std::function<void(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session)>;

    class session_base
    {
    public:
        virtual ~session_base() = default;

        /**
         * Send a raw string to whatever's on the other end of this session
         * @param msg message
         */
        virtual void send_message(std::shared_ptr<bzn::encoded_message> msg) = 0;

        /**
         * Send a message to whatever's on the other end of this session. If the sender field is empty or contains
         * our uuid, the message will be signed before sending. If the sender field contains something else,
         * an existing signature will be kept intact.
         * @param msg           message to send
         */
        virtual void send_signed_message(std::shared_ptr<bzn_envelope> msg) = 0;

        /**
         * Perform an orderly shutdown of the websocket.
         */
        virtual void close() = 0;

        /**
         * Is the underlying socket open? (subject to race conditions)
         */
        virtual bool is_open() const = 0;

        /**
         * Get the id associated with this session
         * @return id
         */
        virtual bzn::session_id get_session_id() = 0;

        /**
         * Create a new websocket connection for this session
         */
        virtual void open(std::shared_ptr<bzn::beast::websocket_base> ws_factory) = 0;

        /**
         * Accept an incoming connection on some websocket
         */
        virtual void accept(std::shared_ptr<bzn::beast::websocket_stream_base> ws) = 0;
    };

} // bzn
