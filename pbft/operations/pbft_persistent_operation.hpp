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

#include <pbft/operations/pbft_operation.hpp>
#include <storage/storage_base.hpp>
#include <proto/pbft.pb.h>
#include <peers_beacon/peers_beacon_base.hpp>

namespace bzn
{
    class pbft_persistent_operation : public pbft_operation
    {
    public:
        pbft_persistent_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash, std::shared_ptr<bzn::storage_base> storage, std::shared_ptr<bzn::peers_beacon_base> peers);

        // constructs operation already in storage
        pbft_persistent_operation(std::shared_ptr<bzn::storage_base> storage, uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash, std::shared_ptr<bzn::peers_beacon_base> peers);

        void record_pbft_msg(const pbft_msg& msg, const bzn_envelope& encoded_msg) override;

        pbft_operation_stage get_stage() const override;
        void advance_operation_stage(pbft_operation_stage new_stage) override;
        bool is_preprepared() const override;
        bool is_prepared() const override;
        bool is_committed() const override;

        bool is_ready_for_commit() const override;
        bool is_ready_for_execute() const override;

        void record_request(const bzn_envelope& encoded_request) override;
        bool has_request() const override;
        bool has_db_request() const override;
        bool has_config_request() const override;

        const bzn_envelope& get_request() const override;
        const pbft_config_msg& get_config_request() const override;
        const database_msg& get_database_msg() const override;

        bzn_envelope get_preprepare() const override;
        std::map<bzn::uuid_t, bzn_envelope> get_prepares() const override;

        static std::string generate_prefix(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash);
        static const std::string& get_uuid();

        static std::vector<std::shared_ptr<pbft_persistent_operation>> prepared_operations_in_range(
            std::shared_ptr<bzn::storage_base> storage, std::shared_ptr<bzn::peers_beacon_base> peers, uint64_t start, std::optional<uint64_t> end = std::nullopt);
        static void remove_range(std::shared_ptr<bzn::storage_base> storage, uint64_t first, uint64_t last);

    private:
        std::string typed_prefix(pbft_msg_type pbft_type) const;
        void load_transient_request() const;

        static std::string prefix_for_sequence(uint64_t sequence);
        static std::string generate_key(const std::string& prefix, const std::string& key);
        static bool parse_prefix(const std::string& prefix, uint64_t& view, uint64_t& sequence, bzn::hash_t& hash);
        std::string increment_prefix(const std::string& prefix) const;

        const std::shared_ptr<bzn::peers_beacon_base> peers;
        const std::shared_ptr<bzn::storage_base> storage;
        const std::string prefix;

        mutable bool transient_request_available = false;
        mutable bzn_envelope transient_request;
        mutable database_msg transient_database_request;
        mutable pbft_config_msg transient_config_request;
    };

}
