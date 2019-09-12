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
#include <peers_beacon/peer_address.hpp>
#include <peers_beacon/peers_beacon_base.hpp>

namespace bzn
{
    class pbft_memory_operation : public pbft_operation
    {
    public:

        pbft_memory_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash, std::shared_ptr<bzn::peers_beacon_base> peers);

        pbft_operation_stage get_stage() const override;

        void record_pbft_msg(const pbft_msg& msg, const bzn_envelope& encoded_msg) override;

        bool is_preprepared() const override;
        bool is_prepared() const override;
        bool is_committed() const override;

        bool is_ready_for_commit() const override;
        bool is_ready_for_execute() const override;

        void advance_operation_stage(pbft_operation_stage new_stage) override;

        void record_request(const bzn_envelope& encoded_request) override;
        bool has_request() const override;
        bool has_db_request() const override;
        bool has_config_request() const override;

        const bzn_envelope& get_request() const override;
        const pbft_config_msg& get_config_request() const override;
        const database_msg& get_database_msg() const override;

        bzn_envelope get_preprepare() const override ;
        std::map<uuid_t, bzn_envelope> get_prepares() const override;

    private:
        bzn_envelope preprepare_message;
        std::map<bzn::uuid_t, bzn_envelope> prepare_messages;

        pbft_operation_stage stage = pbft_operation_stage::prepare;

        bool preprepare_seen = false;
        std::set<bzn::uuid_t> prepares_seen;
        std::set<bzn::uuid_t> commits_seen;

        bzn_envelope request;
        database_msg parsed_db;
        pbft_config_msg parsed_config;

        bool request_saved = false;

        const std::shared_ptr<peers_beacon_base> peers;
    };
}
