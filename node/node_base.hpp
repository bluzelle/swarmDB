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
#include <node/session_base.hpp>
#include <json/json.h>
#include <proto/bluzelle.pb.h>


namespace bzn
{
    class pbft_base;

    class node_base
    {
    public:
        virtual ~node_base() = default;

        /**
         * Register for a callback to be execute when a certain message type arrives
         * @param msg_type      message type (crud, raft etc.)
         * @param msg_handler   callback
         * @return true if registration succeeded
         */
        virtual bool register_for_message(const std::string& msg_type, bzn::message_handler msg_handler) = 0;

        /**
         * Register for a callback to be execute when a certain message type arrives
         * @param msg_type      message type (crud, raft etc.)
         * @param msg_handler   callback
         * @return true if registration succeeded
         */
        virtual bool register_for_message(const bzn_envelope::PayloadCase type, bzn::protobuf_handler msg_handler) = 0;

        /**
         * Start server's listener etc.
         */
        virtual void start(std::shared_ptr<bzn::pbft_base> pbft) = 0;

        /**
         * Convenience method to connect and send a message to a node
         * @param ep            host to send the message to
         * @param msg           message to send
         */
        virtual void send_message_json(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::json_message> msg) = 0;

        /**
         * Convenience method to connect and send a message to a node. Will appropriately populate the sender and
         * signature fields, if not already set.
         * @param ep            host to send the message to
         * @param msg           message to send
         * @param close_session don't expect a response on this session
         */
        virtual void send_message(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn_envelope> msg, bool close_session) = 0;

        /**
         * Convenience method to connect and send a message to a node. Will set sender and signature fields as appropriate.
         * @param ep            host to send the message to
         * @param msg           message to send
         * @param close_session don't expect a response on this session
         */
        virtual void send_message_str(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::encoded_message> msg, bool close_session) = 0;

        /*
         * Convenience method to connect and send a database message to a node by the node's uuid
         * @param uuid          host to send the message to
         * @param msg           message to send
         * @param close_session don't expect a response on this session
         */
        virtual void send_message(const bzn::uuid_t &uuid, std::shared_ptr<bzn_envelope> msg, bool close_session) = 0;
    };

} // bzn
