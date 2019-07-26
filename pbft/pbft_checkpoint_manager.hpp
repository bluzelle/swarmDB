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

#include <storage/storage_base.hpp>
#include <pbft/pbft_persistent_state.hpp>
#include <node/node_base.hpp>
#include <peers_beacon/peers_beacon_base.hpp>
#include <mutex>

namespace bzn
{
    const std::string LATEST_STABLE_CHECKPOINT_KEY{"stable_checkpoint"};
    const std::string STABLE_CHECKPOINT_PROOF_KEY{"stable_checkpoint_proof"};

    const std::string LATEST_LOCAL_CHECKPOINT_KEY{"local_checkpoint"};
    const std::string PARTIAL_CHECKPOINT_PROOFS_KEY{"partial_checkpoint_proofs"};

    const std::string INITIAL_CHECKPOINT_HASH = "<null db state>";

    using checkpoint_t = std::pair<uint64_t, bzn::hash_t>;

class pbft_checkpoint_manager : public std::enable_shared_from_this<pbft_checkpoint_manager>
    {
    public:
        pbft_checkpoint_manager(std::shared_ptr<bzn::asio::io_context_base> io_context,
                std::shared_ptr<bzn::storage_base> storage,
                std::shared_ptr<bzn::peers_beacon_base> peers_beacon,
                std::shared_ptr<bzn::node_base> node);

        void handle_checkpoint_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> /*session*/ = nullptr);
        void local_checkpoint_reached(const checkpoint_t& cp);

        checkpoint_t get_latest_stable_checkpoint() const;
        checkpoint_t get_latest_local_checkpoint() const;

        std::unordered_map<bzn::uuid_t, std::string> get_latest_stable_checkpoint_proof() const;

        size_t partial_checkpoint_proofs_count() const;

        void start();

    private:
        void init_persists();

        void maybe_stabilize_checkpoint(const checkpoint_t& cp);
        void stabilize_checkpoint(const checkpoint_t& cp);

        void send_state_request();
        void send_delayed_state_request(const checkpoint_t& cp);

        std::shared_ptr<bzn::asio::io_context_base> io_context;
        std::shared_ptr<bzn::storage_base> storage;
        std::shared_ptr<bzn::peers_beacon_base> peers_beacon;
        std::shared_ptr<bzn::node_base> node;

        std::unique_ptr<bzn::asio::steady_timer_base> trigger_catchup_timer;

        persistent<checkpoint_t> latest_stable_checkpoint{storage, {0, INITIAL_CHECKPOINT_HASH}, LATEST_STABLE_CHECKPOINT_KEY};
        std::unordered_map<bzn::uuid_t, persistent<std::string>> latest_stable_checkpoint_proof;

        persistent<checkpoint_t> latest_local_checkpoint{storage, {0, INITIAL_CHECKPOINT_HASH}, LATEST_LOCAL_CHECKPOINT_KEY};
        std::map<checkpoint_t, std::unordered_map<bzn::uuid_t, persistent<std::string>>> partial_checkpoint_proofs;

        mutable std::mutex lock;
    };

}
