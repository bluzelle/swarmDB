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
#include <bootstrap/bootstrap_peers.hpp>
#include <status/status_provider_base.hpp>
#include <raft/raft_base.hpp>
#include <raft/log_entry.hpp>
#include <raft/raft_log.hpp>
#include <storage/mem_storage.hpp>
#include <node/node_base.hpp>
#include <gtest/gtest_prod.h>
#include <fstream>
#include <optional>

namespace
{
    const std::string ERROR_ADD_PEER_MUST_BE_SENT_TO_LEADER = "ERROR_ADD_PEER_MUST_BE_SENT_TO_LEADER";
    const std::string ERROR_REMOVE_PEER_MUST_BE_SENT_TO_LEADER = "ERROR_REMOVE_PEER_MUST_BE_SENT_TO_LEADER";
    const std::string MSG_ERROR_CURRENT_QUORUM_IS_JOINT = "A peer cannot be added or removed until the last quorum change request has been processed.";
    const std::string ERROR_PEER_ALREADY_EXISTS = "ERROR_PEER_ALREADY_EXISTS";
    const std::string ERROR_INVALID_UUID = "ERROR_INVALID_UUID";
    const std::string ERROR_PEER_NOT_FOUND = "ERROR_PEER_NOT_FOUND";
    const std::string ERROR_UNABLE_TO_CREATE_LOG_FILE_FOR_WRITING = "Unable to open log file for writing: ";
    const std::string ERROR_PEER_HAS_BEEN_BLACKLISTED = "ERROR_PEER_HAS_BEEN_BLACKLISTED";
    const std::string ERROR_INVALID_SIGNATURE = "ERROR_INVALID_SIGNATURE";
    const std::string ERROR_UNABLE_TO_VALIDATE_UUID ="ERROR_UNABLE_TO_VALIDATE_UUID";
    const std::string SUCCESS_PEER_ADDED_TO_SWARM = "SUCCESS_PEER_ADDED_TO_SWARM";
    const std::string ERROR_GET_PEERS_MUST_BE_SENT_TO_LEADER{"ERROR_GET_PEERS_MUST_BE_SENT_TO_LEADER"};
    const std::string ERROR_GET_PEERS_ELECTION_IN_PROGRESS_TRY_LATER{"ERROR_GET_PEERS_ELECTION_IN_PROGRESS_TRY_LATER"};
    const std::string ERROR_GET_PEERS_SELECTED_NODE_IN_UNKNOWN_STATE{"ERROR_GET_PEERS_SELECTED_NODE_IN_UNKNOWN_STATE"};
}


namespace bzn
{

    class raft final : public bzn::raft_base, public bzn::status_provider_base, public std::enable_shared_from_this<raft>
    {
    public:
        raft(std::shared_ptr<bzn::asio::io_context_base> io_context,
                std::shared_ptr<bzn::node_base> node,
                const bzn::peers_list_t& peers,
                bzn::uuid_t uuid,
                const std::string state_dir,
                size_t maximum_raft_storage = bzn::DEFAULT_MAX_STORAGE_SIZE,
                bool enable_peer_validation = false);

        bzn::raft_state get_state() override;

        bool append_log(const bzn::json_message& msg, const bzn::log_entry_type entry_type) override;

        void register_commit_handler(commit_handler handler) override;

        bzn::peer_address_t get_leader() override;

        void start() override;

        void initialize_storage_from_log(std::shared_ptr<bzn::storage_base> storage);

        bzn::uuid_t get_uuid() { return this->uuid; }

        std::string get_name() override;

        bzn::json_message get_status() override;

        bool get_peer_validation_enabled() override { return this->enable_peer_validation; };

        void set_audit_enabled(bool val);

    private:
        friend class raft_log;
        FRIEND_TEST(raft, test_raft_timeout_scale_can_get_set);
        FRIEND_TEST(raft, test_that_raft_can_rehydrate_state_and_log_entries);
        FRIEND_TEST(raft_test, test_that_raft_can_rehydrate_storage);
        FRIEND_TEST(raft_test, test_that_in_a_leader_state_will_send_a_heartbeat_to_its_peers);
        FRIEND_TEST(raft_test, test_that_leader_sends_entries_and_commits_when_enough_peers_have_saved_them);
        FRIEND_TEST(raft_test, test_that_start_randomly_schedules_callback_for_starting_an_election_and_wins);
        FRIEND_TEST(raft, test_that_raft_bails_on_bad_rehydrate);
        FRIEND_TEST(raft, test_raft_can_find_last_quorum_log_entry);
        FRIEND_TEST(raft_test, test_that_raft_first_log_entry_is_the_quorum);
        FRIEND_TEST(raft, test_raft_can_find_last_quorum_log_entry);
        FRIEND_TEST(raft_test, test_that_raft_first_log_entry_is_the_quorum);
        FRIEND_TEST(raft, test_raft_throws_exception_when_no_quorum_can_be_found_in_log);
        FRIEND_TEST(raft_test, test_that_add_peer_request_to_leader_results_in_correct_joint_quorum);
        FRIEND_TEST(raft_test, test_that_add_or_remove_peer_fails_if_current_quorum_is_a_joint_quorum);
        FRIEND_TEST(raft_test, test_that_remove_peer_request_to_leader_results_in_correct_joint_quorum);
        FRIEND_TEST(raft_test, test_that_add_peer_fails_when_the_peer_uuid_is_already_in_the_single_quorum);
        FRIEND_TEST(raft_test, test_that_remove_peer_fails_when_the_peer_uuid_is_not_in_the_single_quorum);
        FRIEND_TEST(raft, test_raft_can_find_last_quorum_log_entry);
        FRIEND_TEST(raft_test, test_that_raft_first_log_entry_is_the_quorum);
        FRIEND_TEST(raft, test_that_get_all_peers_returns_correct_peers_list_based_on_current_quorum);
        FRIEND_TEST(raft, test_that_is_majority_returns_expected_result_for_single_and_joint_quorums);
        FRIEND_TEST(raft, test_get_active_quorum_returns_single_or_joint_quorum_appropriately);
        FRIEND_TEST(raft_test, test_that_joint_quorum_is_converted_to_single_quorum_and_committed);
        FRIEND_TEST(raft_test, test_that_bad_add_or_remove_peer_requests_fail);
        FRIEND_TEST(raft_test, test_that_a_four_node_swarm_cannot_reach_consensus_with_two_nodes);
        FRIEND_TEST(raft_test, test_that_add_node_works_securely);
        FRIEND_TEST(raft_test, test_that_sending_get_peers_to_follower_fails_and_provides_leader_url);
        FRIEND_TEST(raft_test, test_that_sending_get_peers_to_leader_responds_with_quorum);
        FRIEND_TEST(raft_test, test_that_sending_get_peers_to_candidate_fails);

        bzn::peer_address_t get_leader_unsafe();

        void setup_peer_tracking(const bzn::peers_list_t& peers);

        void start_heartbeat_timer();
        void handle_heartbeat_timeout(const boost::system::error_code& ec);

        void request_append_entries();
        void handle_request_append_entries_response(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session);

        void start_election_timer();
        void handle_election_timeout(const boost::system::error_code& ec);

        void request_vote_request();
        void handle_request_vote_response(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session);

        void handle_ws_raft_messages(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session);
        void handle_ws_request_vote(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session);
        void handle_ws_append_entries(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session);

        void update_raft_state(uint32_t term, bzn::raft_state state);

        void handle_add_peer(std::shared_ptr<bzn::session_base> session, const bzn::json_message& peer);
        void handle_remove_peer(std::shared_ptr<bzn::session_base> session, const std::string& uuid);

        void handle_get_peers(std::shared_ptr<bzn::session_base> session);

        // helpers...
        void get_raft_timeout_scale();

        std::string entries_log_path();

        std::string state_path();

        void import_state_files();
        void create_dat_file(const std::string& log_path, const bzn::peers_list_t& peers);

        void perform_commit(uint32_t& commit_index, const bzn::log_entry& log_entry);
        bool append_log_unsafe(const bzn::json_message& msg, const bzn::log_entry_type entry_type);
        bzn::json_message create_joint_quorum_by_adding_peer(const bzn::json_message& last_quorum_message, const bzn::json_message& new_peer);
        bzn::json_message create_joint_quorum_by_removing_peer(const bzn::json_message& last_quorum_message, const bzn::uuid_t& peer_uuid);
        bzn::json_message create_single_quorum_from_joint_quorum(const bzn::json_message& joint_quorum);

        bool is_majority(const std::set<bzn::uuid_t>& votes);
        uint32_t last_majority_replicated_log_index();
        std::list<std::set<bzn::uuid_t>> get_active_quorum();
        bool in_quorum(const bzn::uuid_t& uuid);
        bzn::peers_list_t get_all_peers();

        void notify_leader_status();
        void notify_commit(size_t log_index, const std::string& operation);
        bzn::log_entry_type deduce_type_from_message(const bzn::json_message& message);

        void shutdown_on_exceeded_max_storage(bool do_throw = false);

        void send_session_error_message(std::shared_ptr<bzn::session_base> session, const std::string& error_message);

        bool validate_new_peer(std::shared_ptr<bzn::session_base> session, const bzn::json_message &peer);

        bzn::json_message to_peer_message(const peer_address_t& address);

        // raft state...
        bzn::raft_state current_state = raft_state::follower;
        uint32_t        current_term = 0;
        std::set<bzn::uuid_t> yes_votes;
        std::set<bzn::uuid_t> no_votes;
        std::optional<bzn::uuid_t> voted_for;
        std::unique_ptr<bzn::asio::steady_timer_base> timer;

        // indexes...
        uint32_t last_log_term  = 0; // not sure if we need this
        uint32_t commit_index   = 1;
        uint32_t timeout_scale  = 1;

        bzn::raft_base::commit_handler commit_handler;

        // track peer's match index...
        std::map<bzn::uuid_t, uint32_t> peer_match_index;

        // misc...
        bzn::uuid_t uuid;
        bzn::uuid_t leader;
        std::shared_ptr<bzn::node_base> node;

        std::once_flag start_once;

        std::mutex raft_lock;

        std::shared_ptr<bzn::raft_log> raft_log;

        const std::string state_dir;

        bool enable_audit = true;

        bool enable_peer_validation{false}; // TODO: RHN - this is only temporary, until the security functionality is tested and in use.
    };
} // bzn
