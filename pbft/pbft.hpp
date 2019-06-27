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
#include <pbft/pbft_base.hpp>
#include <pbft/pbft_failure_detector.hpp>
#include <pbft/pbft_service_base.hpp>
#include <pbft/pbft_persistent_state.hpp>
#include <pbft/pbft_config_store.hpp>
#include <pbft/operations/pbft_operation_manager.hpp>
#include <pbft/pbft_checkpoint_manager.hpp>
#include <storage/storage_base.hpp>
#include <status/status_provider_base.hpp>
#include <crypto/crypto_base.hpp>
#include <proto/audit.pb.h>
#include <monitor/monitor_base.hpp>
#include <mutex>
#include <gtest/gtest_prod.h>
#include <options/options_base.hpp>
#include <pbft/pbft_failure_detector.hpp>
#include <include/boost_asio_beast.hpp>
#include <limits>

namespace
{
    const std::chrono::milliseconds HEARTBEAT_INTERVAL{std::chrono::milliseconds(5000)};
    const std::chrono::seconds NEW_CONFIG_INTERVAL{std::chrono::seconds(30)};
    const std::chrono::seconds JOIN_RETRY_INTERVAL{std::chrono::seconds(30)};
    const uint64_t CHECKPOINT_INTERVAL = 100; //TODO: KEP-574
    const double HIGH_WATER_INTERVAL_IN_CHECKPOINTS = 2.0; //TODO: KEP-574
    const uint64_t MAX_REQUEST_AGE_MS = 300000; // 5 minutes
    const std::string NOOP_REQUEST_HASH = "<no op request hash>";

    const std::string VIEW_KEY{"view"};
    const std::string NEXT_ISSUED_SEQUENCE_NUMBER_KEY{"next_issued_sequence_number"};
    const std::string ACCEPTED_PREPREPARES_KEY{"accepted_preprepares"};
    const std::string VIEW_IS_VALID_KEY{"view_is_valid"};
    const std::string LAST_VIEW_SENT_KEY{"last_view_sent"};
    const std::string VALID_VIEWCHANGE_MESSAGES_FOR_VIEW_KEY{"valid_viewchange_messages_for_view"};
    const std::string SAVED_NEWVIEW_KEY{"saved_newview"};
}


namespace bzn
{
    namespace test
    {
        // fwd declare test as it's not in the same namespace...
        class pbft_test_database_response_is_forwarded_to_session_Test;
        class pbft_test_add_session_to_sessions_waiting_can_add_a_session_and_shutdown_handler_removes_session_from_sessions_waiting_Test;
        class pbft_test_pbft_wrap_message_sets_swarm_id_Test;
        class pbft_test_ensure_save_all_requests_records_requests_Test;
    }

    using request_hash_t = std::string;
    using timestamp_t = uint64_t;

    class pbft final : public bzn::pbft_base, public bzn::status_provider_base, public std::enable_shared_from_this<pbft>
    {
    public:
        pbft(
            std::shared_ptr<bzn::node_base> node
            , std::shared_ptr<bzn::asio::io_context_base> io_context
            , const bzn::peers_list_t& peers
            , std::shared_ptr<bzn::options_base> options
            , std::shared_ptr<pbft_service_base> service
            , std::shared_ptr<pbft_failure_detector_base> failure_detector
            , std::shared_ptr<bzn::crypto_base> crypto
            , std::shared_ptr<bzn::pbft_operation_manager> operation_manager
            , std::shared_ptr<bzn::storage_base> storage
            , std::shared_ptr<bzn::monitor_base> monitor
            );

        void start() override;

        void handle_message(const pbft_msg& msg, const bzn_envelope& original_msg) override;

        void handle_database_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session) override;

        void handle_database_response_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session);

        bool is_primary() const override;

        const peer_address_t& get_primary(std::optional<uint64_t> view = std::nullopt) const override;

        const bzn::uuid_t& get_uuid() const override;

        void handle_failure() override;

        void set_audit_enabled(bool setting);

        uint64_t get_low_water_mark();

        uint64_t get_high_water_mark();

        std::string get_name() override;

        bool is_view_valid() const;

        uint64_t get_view() const;

        bzn::json_message get_status() override;

        bool is_valid_viewchange_message(const pbft_msg& msg, const bzn_envelope& original_msg) const;

        bool is_valid_newview_message(const pbft_msg& theirs, const bzn_envelope& original_theirs) const;

        bool is_valid_prepared_proof(const prepared_proof& proof, uint64_t valid_checkpoint_sequence) const;

        std::shared_ptr<bzn::node_base> get_node();

        const peer_address_t& get_peer_by_uuid(const std::string& uuid) const override;

        /*
         * maximum number of tolerable faults (this can be a parameter, but for now we assume it has the worst-case value)
         * f = floor( (n-1) / 3 )
         */
        static size_t faulty_nodes_bound(size_t swarm_size);

        /*
         * minimum quorum size such that the majority of the quorum is guaranteed to be honest
         * 2f+1
         */
        static size_t honest_majority_size(size_t swarm_size);

        /*
         * minimum quorum size such that at least one member is guaranteed to be honest
         */
        static size_t honest_member_size(size_t swarm_size);

        static uint32_t generate_random_number(uint32_t min, uint32_t max);

        std::shared_ptr<const std::vector<bzn::peer_address_t>> current_peers_ptr() const override;

        checkpoint_t latest_stable_checkpoint() const;

    private:
        bool preliminary_filter_msg(const pbft_msg& msg);

        void handle_request(const bzn_envelope& request, const std::shared_ptr<session_base>& session = nullptr);
        void handle_preprepare(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_prepare(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_commit(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_checkpoint(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_join_or_leave(const bzn_envelope& env, const pbft_membership_msg& msg, std::shared_ptr<bzn::session_base> session, const std::string& msg_hash);
        void handle_join_response(const pbft_membership_msg& msg);
        void handle_get_state(const pbft_membership_msg& msg, std::shared_ptr<bzn::session_base> session) const;
        void handle_set_state(const pbft_membership_msg& msg);
        void handle_config_message(const pbft_msg& msg, const std::shared_ptr<pbft_operation>& op);
        void handle_viewchange(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_newview(const pbft_msg& msg, const bzn_envelope& original_msg);

        void maybe_advance_operation_state(const std::shared_ptr<pbft_operation>& op);
        void do_preprepare(const std::shared_ptr<pbft_operation>& op);
        void do_preprepared(const std::shared_ptr<pbft_operation>& op);
        void do_prepared(const std::shared_ptr<pbft_operation>& op);
        void do_committed(const std::shared_ptr<pbft_operation>& op);

        void handle_bzn_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session);
        void handle_membership_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session = nullptr);

        bzn_envelope wrap_message(const pbft_msg& message) const;
        bzn_envelope wrap_message(const pbft_membership_msg&) const;

        void async_signed_broadcast(const pbft_msg& msg);
        void async_signed_broadcast(const pbft_membership_msg& message);
        void async_signed_broadcast(const audit_message& message);
        void async_signed_broadcast(std::shared_ptr<bzn_envelope> message);

        pbft_msg common_message_setup(const std::shared_ptr<pbft_operation>& op, pbft_msg_type type);
        std::shared_ptr<pbft_operation> setup_request_operation(const bzn_envelope& msg
            , const bzn::hash_t& request_hash);
        void forward_request_to_primary(const bzn_envelope& request_env);

        void broadcast(const bzn_envelope& message);

        void handle_audit_heartbeat_timeout(const boost::system::error_code& ec);
        void handle_new_config_timeout(const boost::system::error_code& ec);
        void handle_join_retry_timeout(const boost::system::error_code& ec);

        void notify_audit_failure_detected();

        std::shared_ptr<std::string> get_checkpoint_state(const checkpoint_t& cp) const;
        void set_checkpoint_state(const checkpoint_t& cp, const std::string& data);

        inline size_t quorum_size() const;
        size_t max_faulty_nodes() const;

        void initialize_persistent_state();
        bool initialize_configuration(const bzn::peers_list_t& peers);
        const std::vector<bzn::peer_address_t>& current_peers() const;

        void broadcast_new_configuration(pbft_configuration::shared_const_ptr config, const std::string& join_request_hash);
        bool is_configuration_acceptable_in_new_view(const hash_t& config_hash);
        bool move_to_new_configuration(const hash_t& config_hash, uint64_t view);
        bool proposed_config_is_acceptable(std::shared_ptr<pbft_configuration> config);

        void maybe_record_request(const bzn_envelope &env, const std::shared_ptr<pbft_operation> &op);

        timestamp_t now() const;
        bool already_seen_request(const bzn_envelope& msg, const request_hash_t& hash) const;
        void saw_request(const bzn_envelope& msg, const request_hash_t& hash);

        void join_swarm();

        // VIEWCHANGE/NEWVIEW Helper methods
        void initiate_viewchange();
        std::shared_ptr<bzn_envelope> make_viewchange(uint64_t new_view, uint64_t n, const std::unordered_map<bzn::uuid_t, std::string>& stable_checkpoint_proof, const std::map<uint64_t, std::shared_ptr<bzn::pbft_operation>>& prepared_operations);
        pbft_msg make_newview(uint64_t new_view_index,  const std::map<uuid_t,bzn_envelope>& viewchange_envelopes_from_senders, const std::map<uint64_t, bzn_envelope>& pre_prepare_messages) const;
        pbft_msg build_newview(uint64_t new_view, const std::map<uuid_t,bzn_envelope>& viewchange_envelopes_from_senders) const;
        std::map<bzn::checkpoint_t , std::set<bzn::uuid_t>> validate_and_extract_checkpoint_hashes(const pbft_msg &viewchange_message) const;
        void save_checkpoint(const pbft_msg& msg);
        void fill_in_missing_pre_prepares(uint64_t max_checkpoint_sequence, uint64_t new_view, std::map<uint64_t, bzn_envelope>& pre_prepares) const;
        bool is_peer(const bzn::uuid_t& peer) const;
        bool get_sequences_and_request_hashes_from_proofs( const pbft_msg& viewchange_msg, std::set<std::pair<uint64_t, std::string>>& sequence_request_pairs) const;

        void add_session_to_sessions_waiting(const std::string &msg_hash, std::shared_ptr<bzn::session_base> session);

        std::map<bzn::hash_t, int> map_request_to_hash(const bzn_envelope& original_msg);
        void save_all_requests(const pbft_msg& msg, const bzn_envelope& original_msg);


        std::shared_ptr<bzn::storage_base> storage;

        // Using 1 as first value here to distinguish from default value of 0 in protobuf
        persistent<uint64_t> view{storage, uint64_t{1}, VIEW_KEY};
        persistent<uint64_t> next_issued_sequence_number{storage, 1, NEXT_ISSUED_SEQUENCE_NUMBER_KEY};

        std::shared_ptr<bzn::node_base> node;

        const bzn::uuid_t uuid;
        std::shared_ptr<bzn::options_base> options;

        std::shared_ptr<pbft_service_base> service;

        std::shared_ptr<pbft_failure_detector_base> failure_detector;

        std::mutex pbft_lock;

        std::map<bzn::log_key_t, persistent<bzn::operation_key_t>> accepted_preprepares;

        std::once_flag start_once;

        const std::shared_ptr<bzn::asio::io_context_base> io_context;

        std::unique_ptr<bzn::asio::steady_timer_base> audit_heartbeat_timer;
        std::unique_ptr<bzn::asio::steady_timer_base> new_config_timer;
        std::unique_ptr<bzn::asio::steady_timer_base> join_retry_timer;

        bool audit_enabled = true;

        enum class swarm_status {not_joined, joining, waiting, joined};
        swarm_status in_swarm = swarm_status::not_joined;

        bool new_config_in_flight = false;

        std::multimap<timestamp_t, std::pair<bzn::uuid_t, request_hash_t>> recent_requests;

        std::shared_ptr<crypto_base> crypto;

        // VIEWCHANGE/NEWVIEW members
        persistent<bool> view_is_valid{storage, true, VIEW_IS_VALID_KEY};
        persistent<uint64_t> last_view_sent{storage, 0, LAST_VIEW_SENT_KEY};

        std::map<uint64_t,std::map<bzn::uuid_t, persistent<bzn_envelope>>> valid_viewchange_messages_for_view; // set of bzn_envelope, strings since we cannot have a set<bzn_envelope>
        bzn_envelope saved_newview;

        std::shared_ptr<pbft_operation_manager> operation_manager;
        std::shared_ptr<pbft_config_store> configurations;
        std::shared_ptr<bzn::pbft_checkpoint_manager> checkpoint_manager;
        std::shared_ptr<bzn::monitor_base> monitor;

        FRIEND_TEST(pbft_viewchange_test, pbft_with_invalid_view_drops_messages);
        FRIEND_TEST(pbft_viewchange_test, test_make_signed_envelope);
        FRIEND_TEST(pbft_viewchange_test, test_is_peer);
        FRIEND_TEST(pbft_viewchange_test, validate_and_extract_checkpoint_hashes);
        FRIEND_TEST(pbft_viewchange_test, validate_viewchange_checkpoints);
        FRIEND_TEST(pbft_viewchange_test, test_is_valid_viewchange_message);
        FRIEND_TEST(pbft_viewchange_test, make_viewchange_makes_valid_message);
        FRIEND_TEST(pbft_viewchange_test, test_prepared_operations_since_last_checkpoint);
        FRIEND_TEST(pbft_viewchange_test, test_fill_in_missing_pre_prepares);
        FRIEND_TEST(pbft_viewchange_test, test_save_checkpoint);
        FRIEND_TEST(pbft_viewchange_test, test_handle_viewchange);
        FRIEND_TEST(pbft_viewchange_test, is_valid_viewchange_does_not_throw_if_no_checkpoint_yet);

        FRIEND_TEST(pbft_newview_test, test_pre_prepares_contiguous);
        FRIEND_TEST(pbft_newview_test, make_newview);
        FRIEND_TEST(pbft_newview_test, build_newview);
        FRIEND_TEST(pbft_newview_test, primary_handle_newview);
        FRIEND_TEST(pbft_newview_test, backup_handle_newview);
        FRIEND_TEST(pbft_newview_test, validate_and_extract_checkpoint_hashes);
        FRIEND_TEST(pbft_newview_test, test_get_primary);
        FRIEND_TEST(pbft_newview_test, get_sequences_and_request_hashes_from_proofs);
        FRIEND_TEST(pbft_newview_test, test_last_sequence_in_newview_prepared_proofs);
        FRIEND_TEST(bzn::test::pbft_test, database_response_is_forwarded_to_session);
        FRIEND_TEST(bzn::test::pbft_test, add_session_to_sessions_waiting_can_add_a_session_and_shutdown_handler_removes_session_from_sessions_waiting);
        FRIEND_TEST(bzn::test::pbft_test, pbft_wrap_message_sets_swarm_id);
        FRIEND_TEST(bzn::test::pbft_test, ensure_save_all_requests_records_requests);

        friend class pbft_proto_test;
        friend class pbft_join_leave_test;
        friend class pbft_viewchange_test;

        std::map<bzn::hash_t, std::shared_ptr<bzn::session_base>> sessions_waiting_on_forwarded_requests;
    };

} // namespace bzn
