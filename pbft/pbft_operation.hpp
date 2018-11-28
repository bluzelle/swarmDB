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
#include <proto/pbft.pb.h>
#include <bootstrap/bootstrap_peers_base.hpp>
#include <cstdint>
#include <string>
#include <node/session_base.hpp>

namespace bzn
{
    // View, sequence
    using operation_key_t = std::tuple<uint64_t, uint64_t, hash_t>;

    // View, sequence
    using log_key_t = std::tuple<uint64_t, uint64_t>;

    enum class pbft_operation_state
    {
        prepare, commit, committed
    };


    class pbft_operation
    {
    public:

        pbft_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash, std::shared_ptr<const std::vector<peer_address_t>> peers);

        void set_session(std::shared_ptr<bzn::session_base>);
        std::shared_ptr<bzn::session_base> session() const;
        bool has_session() const;

        operation_key_t get_operation_key() const;
        pbft_operation_state get_state() const;

        void record_preprepare(const bzn_envelope& encoded_preprepare);
        bool has_preprepare() const;

        void record_prepare(const bzn_envelope& encoded_prepare);
        bool is_prepared() const;

        void record_commit(const bzn_envelope& encoded_commit);
        bool is_committed() const;

        void begin_commit_phase();
        void end_commit_phase();

        void record_request(const bzn_envelope& encoded_request);
        bool has_request() const;
        bool has_db_request() const;
        bool has_config_request() const;

        const bzn_envelope& get_request() const;
        const pbft_config_msg& get_config_request() const;
        const database_msg& get_database_msg() const;

        const uint64_t view;
        const uint64_t sequence;
        const bzn::hash_t request_hash;

        std::string debug_string() const;

        size_t faulty_nodes_bound() const;

    private:
        const std::shared_ptr<const std::vector<peer_address_t>> peers;

        pbft_operation_state state = pbft_operation_state::prepare;

        bool preprepare_seen = false;
        std::set<bzn::uuid_t> prepares_seen;
        std::set<bzn::uuid_t> commits_seen;

        std::shared_ptr<bzn::session_base> listener_session;

        bzn_envelope request;
        database_msg parsed_db;
        pbft_config_msg parsed_config;

        bool request_saved = false;
        bool session_saved = false;
    };
}
