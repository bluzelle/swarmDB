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

#include <chaos/chaos_base.hpp>
#include <crypto/crypto_base.hpp>
#include <include/boost_asio_beast.hpp>
#include <monitor/monitor_base.hpp>
#include <node/node_base.hpp>
#include <options/options_base.hpp>
#include <pbft/pbft_base.hpp>
#include <atomic>
#include <json/json.h>
#include <shared_mutex>


#include <gtest/gtest_prod.h>


namespace bzn
{
    using ep_key_t = std::string;

    class node final : public bzn::node_base, public std::enable_shared_from_this<node>
    {
    public:
        node(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::beast::websocket_base> websocket, std::shared_ptr<chaos_base> chaos,
                   const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::crypto_base> crypto, std::shared_ptr<bzn::options_base> options, std::shared_ptr<monitor_base> monitor);

        bool register_for_message(const bzn_envelope::PayloadCase type, bzn::protobuf_handler msg_handler) override;

        void start(std::shared_ptr<bzn::pbft_base> pbft) override;

        void send_signed_message(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn_envelope> msg) override;

        void multicast_signed_message(std::shared_ptr<std::vector<boost::asio::ip::tcp::endpoint>> eps, std::shared_ptr<bzn_envelope> msg) override;

        void send_message_str(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::encoded_message> msg) override;

        void send_signed_message(const bzn::uuid_t& uuid, std::shared_ptr<bzn_envelope> msg) override;

    private:
        FRIEND_TEST(node, test_that_registered_message_handler_is_invoked);
        FRIEND_TEST(node, test_that_wrongly_signed_messages_are_dropped);

        void do_accept();

        void priv_protobuf_handler(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session);
        void priv_session_shutdown_handler(const ep_key_t& ep_key);

        void initialize_ssl_contexts();

        std::shared_ptr<bzn::pbft_base> pbft;

        std::shared_ptr<bzn::session_base> find_session(const boost::asio::ip::tcp::endpoint& ep);

        ep_key_t key_from_ep(const boost::asio::ip::tcp::endpoint& ep);

        std::unordered_map<ep_key_t, std::weak_ptr<bzn::session_base>> sessions;
        std::mutex session_map_mutex;

        std::unique_ptr<bzn::asio::tcp_acceptor_base> tcp_acceptor;
        std::shared_ptr<bzn::asio::io_context_base>   io_context;
        std::unique_ptr<bzn::asio::tcp_socket_base>   acceptor_socket;
        std::shared_ptr<bzn::beast::websocket_base>   websocket;
        std::shared_ptr<bzn::chaos_base>              chaos;

        std::unordered_map<bzn_envelope::PayloadCase, bzn::protobuf_handler> protobuf_map;
        std::shared_mutex message_map_mutex;

        std::once_flag start_once;

        std::atomic<bzn::session_id> session_id_counter = 0;

        std::shared_ptr<bzn::crypto_base> crypto;
        std::shared_ptr<bzn::options_base> options;
        std::shared_ptr<bzn::monitor_base> monitor;

        std::shared_ptr<boost::asio::ssl::context> server_ctx;
        std::shared_ptr<boost::asio::ssl::context> client_ctx;

    };

} // bzn
