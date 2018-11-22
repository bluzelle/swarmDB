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
#include <pbft/pbft_config_store.hpp>
#include <status/status_provider_base.hpp>
#include <crypto/crypto_base.hpp>
#include <proto/audit.pb.h>
#include <mutex>
#include <gtest/gtest_prod.h>
#include <pbft/pbft_failure_detector.hpp>
#include <include/boost_asio_beast.hpp>
#include <limits>

namespace
{
    const std::chrono::milliseconds HEARTBEAT_INTERVAL{std::chrono::milliseconds(5000)};
    const std::string INITIAL_CHECKPOINT_HASH = "<null db state>";
    const uint64_t CHECKPOINT_INTERVAL = 100; //TODO: KEP-574
    const double HIGH_WATER_INTERVAL_IN_CHECKPOINTS = 2.0; //TODO: KEP-574
    const uint64_t MAX_REQUEST_AGE_MS = 300000; // 5 minutes
}

namespace bzn
{
    using request_hash_t = std::string;
    using checkpoint_t = std::pair<uint64_t, bzn::hash_t>;
    using timestamp_t = uint64_t;

    class pbft final : public bzn::pbft_base, public bzn::status_provider_base, public std::enable_shared_from_this<pbft>
    {
    public:
        pbft(
            std::shared_ptr<bzn::node_base> node
            , std::shared_ptr<bzn::asio::io_context_base> io_context
            , const bzn::peers_list_t& peers
            , bzn::uuid_t uuid
            , std::shared_ptr<pbft_service_base> service
            , std::shared_ptr<pbft_failure_detector_base> failure_detector
            , std::shared_ptr<bzn::crypto_base> crypto
            );

        void start() override;

        void handle_message(const pbft_msg& msg, const bzn_envelope& original_msg) override;

        void handle_database_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session);

        size_t outstanding_operations_count() const;

        bool is_primary() const override;

        const peer_address_t& get_primary(std::optional<uint64_t> view = std::nullopt) const override;

        const bzn::uuid_t& get_uuid() const override;

        void handle_failure() override;

        void set_audit_enabled(bool setting);

        checkpoint_t latest_stable_checkpoint() const;

        checkpoint_t latest_checkpoint() const;

        size_t unstable_checkpoints_count() const;

        uint64_t get_low_water_mark();

        uint64_t get_high_water_mark();

        std::string get_name() override;

        bool is_view_valid() const;

        uint64_t get_view() const { return this->view; }

        bzn::json_message get_status() override;

        bool is_valid_viewchange_message(const pbft_msg& msg, const bzn_envelope& original_msg) const;

        bool is_valid_newview_message(const pbft_msg& msg, const bzn_envelope& original_msg) const;



    private:
        std::shared_ptr<pbft_operation> find_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash);
        std::shared_ptr<pbft_operation> find_operation(const pbft_msg& msg);
        std::shared_ptr<pbft_operation> find_operation(const std::shared_ptr<pbft_operation>& op);

        bool preliminary_filter_msg(const pbft_msg& msg);

        void handle_request(const bzn_envelope& request, const std::shared_ptr<session_base>& session = nullptr);
        void handle_preprepare(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_prepare(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_commit(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_checkpoint(const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_join_or_leave(const pbft_membership_msg& msg);
        void handle_get_state(const pbft_membership_msg& msg, std::shared_ptr<bzn::session_base> session) const;
        void handle_set_state(const pbft_membership_msg& msg);
        void handle_config_message(const pbft_msg& msg, const std::shared_ptr<pbft_operation>& op);
        void handle_viewchange    (const pbft_msg& msg, const bzn_envelope& original_msg);
        void handle_newview       (const pbft_msg& msg, const bzn_envelope& original_msg);

        void maybe_advance_operation_state(const std::shared_ptr<pbft_operation>& op);
        void do_preprepare(const std::shared_ptr<pbft_operation>& op);
        void do_preprepared(const std::shared_ptr<pbft_operation>& op);
        void do_prepared(const std::shared_ptr<pbft_operation>& op);
        void do_committed(const std::shared_ptr<pbft_operation>& op);

        void handle_bzn_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session);
        void handle_membership_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session = nullptr);
        bzn_envelope wrap_message(const pbft_msg& message, const std::string& debug_info = "");
        bzn_envelope wrap_message(const pbft_membership_msg& message, const std::string& debug_info = "") const;
        bzn::encoded_message wrap_message(const audit_message& message, const std::string& debug_info = "");
        
        pbft_msg common_message_setup(const std::shared_ptr<pbft_operation>& op, pbft_msg_type type);
        std::shared_ptr<pbft_operation> setup_request_operation(const bzn_envelope& msg
            , const bzn::hash_t& request_hash
            , const std::shared_ptr<session_base>& session = nullptr);

        void broadcast(const bzn_envelope& message);
        void broadcast(const bzn::encoded_message& message);

        void handle_audit_heartbeat_timeout(const boost::system::error_code& ec);

        void notify_audit_failure_detected();

        void checkpoint_reached_locally(uint64_t sequence);
        void maybe_stabilize_checkpoint(const checkpoint_t& cp);
        void stabilize_checkpoint(const checkpoint_t& cp);
        const peer_address_t& select_peer_for_checkpoint(const checkpoint_t& cp);
        void request_checkpoint_state(const checkpoint_t& cp);
        std::string get_checkpoint_state(const checkpoint_t& cp) const;
        void set_checkpoint_state(const checkpoint_t& cp, const std::string& data);

        inline size_t quorum_size() const;
        size_t max_faulty_nodes() const;

        void clear_local_checkpoints_until(const checkpoint_t&);
        void clear_checkpoint_messages_until(const checkpoint_t&);
        void clear_operations_until(const checkpoint_t&);

        bool initialize_configuration(const bzn::peers_list_t& peers);
        std::shared_ptr<const std::vector<bzn::peer_address_t>> current_peers_ptr() const;
        const std::vector<bzn::peer_address_t>& current_peers() const;
        const peer_address_t& get_peer_by_uuid(const std::string& uuid) const;
        void broadcast_new_configuration(pbft_configuration::shared_const_ptr config);
        bool is_configuration_acceptable_in_new_view(hash_t config_hash);
        bool move_to_new_configuration(hash_t config_hash);
        bool proposed_config_is_acceptable(std::shared_ptr<pbft_configuration> config);

        void maybe_record_request(const pbft_msg& msg, const std::shared_ptr<pbft_operation>& op);

        timestamp_t now() const;
        bool already_seen_request(const bzn_envelope& msg, const request_hash_t& hash) const;
        void saw_request(const bzn_envelope& msg, const request_hash_t& hash);


        // VIEWCHANGE/NEWVIEW Helper methods
        static pbft_msg make_viewchange(uint64_t new_view, uint64_t n, std::unordered_map<bzn::uuid_t, std::string> stable_checkpoint_proof, std::set<std::shared_ptr<bzn::pbft_operation>> prepared_operations);
        pbft_msg make_newview(uint64_t new_view_index, const std::vector<pbft_msg> &view_change_messages, const std::map<uint64_t, bzn_envelope> &pre_prepare_messages);
        pbft_msg build_newview(uint64_t new_view, const std::vector<pbft_msg> &viewchange_messages);
        bzn_envelope make_signed_envelope(std::string serialized_pbft_message);
        std::set<std::shared_ptr<bzn::pbft_operation>> prepared_operations_since_last_checkpoint();
        std::optional<bzn::checkpoint_t> validate_viewchange_checkpoints(const pbft_msg &viewchange_message) const;
        std::map<bzn::checkpoint_t , std::set<bzn::uuid_t>> validate_and_extract_checkpoint_hashes(const pbft_msg &viewchange_message) const;
        void save_checkpoint(const pbft_msg& msg);
        void fill_in_missing_pre_prepares(std::map<uint64_t, bzn_envelope> &pre_prepares);
        bool is_peer(const bzn::uuid_t& peer) const;
        bool get_sequences_and_request_hashes_from_proofs( const pbft_msg& viewchange_msg, std::set<std::pair<uint64_t, std::string>>& sequence_request_pairs) const;
        void replica_broadcasts_viewchange(const pbft_msg& msg);


        // Using 1 as first value here to distinguish from default value of 0 in protobuf
        uint64_t view = 1;
        uint64_t next_issued_sequence_number = 1;

        uint64_t low_water_mark;
        uint64_t high_water_mark;

        std::shared_ptr<bzn::node_base> node;

        const bzn::uuid_t uuid;

        std::shared_ptr<pbft_service_base> service;

        std::shared_ptr<pbft_failure_detector_base> failure_detector;

        std::mutex pbft_lock;

        std::map<bzn::operation_key_t, std::shared_ptr<bzn::pbft_operation>> operations;

        std::map<bzn::log_key_t, bzn::operation_key_t> accepted_preprepares;

        std::once_flag start_once;

        const std::shared_ptr<bzn::asio::io_context_base> io_context;

        std::unique_ptr<bzn::asio::steady_timer_base> audit_heartbeat_timer;

        bool audit_enabled = true;

        checkpoint_t stable_checkpoint{0, INITIAL_CHECKPOINT_HASH};

        std::unordered_map<uuid_t, std::string> stable_checkpoint_proof;

        std::set<checkpoint_t> local_unstable_checkpoints;

        std::map<checkpoint_t, std::unordered_map<uuid_t, std::string>> unstable_checkpoint_proofs;

        pbft_config_store configurations;

        std::multimap<timestamp_t, std::pair<bzn::uuid_t, request_hash_t>> recent_requests;

        std::shared_ptr<crypto_base> crypto;

        // VIEWCHANGE/NEWVIEW members
        bool view_is_valid = true;
        uint64_t last_view_sent{0};
        std::map<uint64_t, std::set<std::string>> valid_view_change_messages; // set of bzn_envelope, strings since we cannot have a set<bzn_envelope>

        FRIEND_TEST(pbft_test, join_request_generates_new_config_preprepare);
        FRIEND_TEST(pbft_test, valid_leave_request_test);
        FRIEND_TEST(pbft_test, invalid_leave_request_test);
        FRIEND_TEST(pbft_test, test_new_config_preprepare_handling);
        FRIEND_TEST(pbft_test, test_new_config_prepare_handling);
        FRIEND_TEST(pbft_test, test_new_config_commit_handling);
        FRIEND_TEST(pbft_test, test_move_to_new_config);
        FRIEND_TEST(pbft_test, full_test);

        // good tests
        FRIEND_TEST(pbft_viewchange_test, test_make_signed_envelope);
        FRIEND_TEST(pbft_viewchange_test, test_is_peer);
        FRIEND_TEST(pbft_viewchange_test, validate_viewchange_checkpoints);

        // bad tests
        FRIEND_TEST(pbft_viewchange_test, make_viewchange_makes_valid_message);
        FRIEND_TEST(pbft_viewchange_test, primary_handle_viewchange);
        FRIEND_TEST(pbft_viewchange_test, backup_handle_viewchange);
        FRIEND_TEST(pbft_viewchange_test, make_viewchange_message);
        FRIEND_TEST(pbft_viewchange_test, test_prepared_operations_since_last_checkpoint);

        FRIEND_TEST(pbft_newview_test, make_signed_envelope);
        FRIEND_TEST(pbft_newview_test, make_newview_makes_valid_message);
        FRIEND_TEST(pbft_newview_test, make_newview);
        FRIEND_TEST(pbft_newview_test, build_newview);
        FRIEND_TEST(pbft_newview_test, primary_handle_newview);
        FRIEND_TEST(pbft_newview_test, backup_handle_newview);
        FRIEND_TEST(pbft_newview_test, validate_and_extract_checkpoint_hashes);
        FRIEND_TEST(pbft_newview_test, test_validate_preprepare_sequences);
        FRIEND_TEST(pbft_newview_test, test_get_primary);
        // end of bad tests

        friend class pbft_proto_test;

        std::shared_ptr<crypto_base> crypto;

        std::map<bzn::hash_t, std::weak_ptr<bzn::session_base>> sessions_waiting_on_forwarded_requests;

        std::map<uint64_t, std::set<std::string>> valid_view_change_messages; // set of bzn_envelope, strings since we cannot have a set<bzn_envelope>

        static pbft_msg make_viewchange(
                uint64_t new_view
                , uint64_t n
                , std::unordered_map<bzn::uuid_t, std::string> stable_checkpoint_proof
                , std::set<std::shared_ptr<bzn::pbft_operation>> prepared_operations);

        pbft_msg make_newview(
                uint64_t new_view_index, const std::vector<pbft_msg> &view_change_messages,
                const std::map<uint64_t, bzn_envelope> &pre_prepare_messages
        );

        pbft_msg build_newview(uint64_t new_view, const std::vector<pbft_msg> &viewchange_messages);

        bzn_envelope make_signed_envelope(std::string serialized_pbft_message);

        std::optional<bzn::checkpoint_t> validate_viewchange_checkpoints(const pbft_msg &viewchange_message) const;

        std::map<bzn::checkpoint_t , std::set<bzn::uuid_t>> validate_and_extract_checkpoint_hashes(
                const pbft_msg &viewchange_message) const;
        void save_checkpoint(const pbft_msg& msg);

        void fill_in_missing_pre_prepares(std::map<uint64_t, bzn_envelope> &pre_prepares);

        bool is_peer(const bzn::uuid_t& peer) const;

        bool get_sequences_and_request_hashes_from_proofs(
                const pbft_msg& viewchange_msg
                , std::set<std::pair<uint64_t, std::string>>& sequence_request_pairs) const;
    };

} // namespace bzn
