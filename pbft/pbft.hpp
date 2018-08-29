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
#include <pbft/pbft_base.hpp>
#include <pbft/pbft_service_base.hpp>
#include <mutex>
#include <pbft/pbft_failure_detector.hpp>
#include <include/boost_asio_beast.hpp>

namespace
{
    const uint64_t CHECKPOINT_INTERVAL = 100; //TODO: KEP-574
    const std::string INITIAL_CHECKPOINT_HASH = "<null db state>";
}

namespace bzn
{

    using request_hash_t = std::string;
    using checkpoint_t = std::pair<uint64_t, bzn::hash_t>;

    class pbft final : public bzn::pbft_base, public std::enable_shared_from_this<pbft>
    {
    public:
        pbft(
                std::shared_ptr<bzn::node_base> node
                , std::shared_ptr<bzn::asio::io_context_base> io_context
                , const bzn::peers_list_t& peers
                , bzn::uuid_t uuid
                , std::shared_ptr<pbft_service_base> service
                , std::shared_ptr<pbft_failure_detector_base> failure_detector
        );

        void start() override;

        void handle_message(const pbft_msg& msg) override;

        size_t outstanding_operations_count() const;

        bool is_primary() const override;

        const peer_address_t& get_primary() const override;

        const bzn::uuid_t& get_uuid() const override;

        void handle_failure() override;

        void set_audit_enabled(bool setting);

        checkpoint_t latest_stable_checkpoint() const;
        checkpoint_t latest_checkpoint() const;
        size_t unstable_checkpoints_count() const;


    private:
        std::shared_ptr<pbft_operation> find_operation(uint64_t view, uint64_t sequence, const pbft_request& request);
        std::shared_ptr<pbft_operation> find_operation(const pbft_msg& msg);

        bzn::hash_t request_hash(const pbft_request& req);

        bool preliminary_filter_msg(const pbft_msg& msg);

        void handle_request(const pbft_msg& msg);
        void handle_preprepare(const pbft_msg& msg);
        void handle_prepare(const pbft_msg& msg);
        void handle_commit(const pbft_msg& msg);
        void handle_checkpoint(const pbft_msg& msg);

        void maybe_advance_operation_state(const std::shared_ptr<pbft_operation>& op);
        void do_preprepare(const std::shared_ptr<pbft_operation>& op);
        void do_preprepared(const std::shared_ptr<pbft_operation>& op);
        void do_prepared(const std::shared_ptr<pbft_operation>& op);
        void do_committed(const std::shared_ptr<pbft_operation>& op);

        void unwrap_message(const bzn::message& json, std::shared_ptr<bzn::session_base> session);
        bzn::message wrap_message(const pbft_msg& message, const std::string& debug_info = "");
        bzn::message wrap_message(const audit_message& message, const std::string& debug_info = "");
        
        pbft_msg common_message_setup(const std::shared_ptr<pbft_operation>& op, pbft_msg_type type);

        void broadcast(const bzn::message& message);

        void handle_audit_heartbeat_timeout(const boost::system::error_code& ec);

        void notify_audit_failure_detected();

        void checkpoint_reached_locally(uint64_t sequence);
        void maybe_stabilize_checkpoint(const checkpoint_t& cp);

        inline size_t quorum_size() const;
        inline size_t max_faulty_nodes() const;

        // Using 1 as first value here to distinguish from default value of 0 in protobuf
        uint64_t view = 1;
        uint64_t next_issued_sequence_number = 1;

        uint64_t low_water_mark = 1;
        uint64_t high_water_mark = UINT64_MAX;

        std::shared_ptr<bzn::node_base> node;

        std::vector<bzn::peer_address_t> peer_index;

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
    };

} // namespace bzn

