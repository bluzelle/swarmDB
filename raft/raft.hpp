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
#include <include/boost_asio_beast.hpp>
#include <bootstrap/bootstrap_peers.hpp>
#include <raft/raft_base.hpp>
#include <node/node_base.hpp>
#include <memory>
#ifndef __APPLE__
#include <optional>
#else
#include <experimental/optional>
#endif

#include <gtest/gtest_prod.h>

namespace bzn
{
    class raft final : public bzn::raft_base, public std::enable_shared_from_this<raft>
    {
    public:
        raft(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::node_base> node,
             const bzn::peers_list_t& peers, bzn::uuid_t uuid);

        bzn::raft_state get_state() override;

        bool append_log(const bzn::message& msg) override;

        void register_commit_handler(commit_handler handler) override;

        bzn::peer_address_t get_leader() override;

        void start() override;

    private:
        FRIEND_TEST(raft, test_raft_timeout_scale_can_get_set);

        void start_heartbeat_timer();
        void handle_heartbeat_timeout(const boost::system::error_code& ec);

        void request_append_entries();
        void handle_request_append_entries_response(const bzn::message& msg, std::shared_ptr <bzn::session_base> session);

        void start_election_timer();
        void handle_election_timeout(const boost::system::error_code& ec);

        void request_vote_request();
        void handle_request_vote_response(const bzn::message& msg, std::shared_ptr <bzn::session_base> session);

        void handle_ws_raft_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session);
        void handle_ws_request_vote(const bzn::message& msg, std::shared_ptr<bzn::session_base> session);
        void handle_ws_append_entries(const bzn::message& msg, std::shared_ptr<bzn::session_base> session);

        void update_raft_state(uint32_t term, bzn::raft_state state);

        // helpers...
        void get_raft_timeout_scale();

        // raft state...
        bzn::raft_state current_state = raft_state::follower;
        uint32_t        current_term = 1;
        std::size_t     yes_votes = 0;
        std::size_t     no_votes  = 0;
#ifndef __APPLE__
        std::optional<bzn::uuid_t> voted_for;
#else
        std::experimental::optional<bzn::uuid_t> voted_for;
#endif
        std::unique_ptr<bzn::asio::steady_timer_base> timer;

        // indexes...
        uint32_t last_log_index = 0;
        uint32_t last_log_term  = 0; // not sure if we need this
        uint32_t commit_index   = 0;
        uint32_t timeout_scale  = 1;

        // log_entries entries...
        struct log_entry
        {
            uint32_t     log_index;
            uint32_t     term;
            bzn::message msg;
        };

        std::vector<log_entry> log_entries;
        bzn::raft_base::commit_handler commit_handler;

        // track peer's match index...
        std::map<bzn::uuid_t, uint32_t> peer_match_index;

        // misc...
        const bzn::peers_list_t peers;
        bzn::uuid_t uuid;
        bzn::uuid_t leader;
        std::shared_ptr<bzn::node_base> node;

        std::once_flag start_once;
    };

} // bzn
