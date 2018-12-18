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

#include <proto/pbft.pb.h>
#include <node/session_base.hpp>
#include <include/bluzelle.hpp>
#include <cstdint>

namespace bzn
{
    // View, sequence, hash
    using operation_key_t = std::tuple<uint64_t, uint64_t, hash_t>;

    // View, sequence
    using log_key_t = std::tuple<uint64_t, uint64_t>;

    enum class pbft_operation_stage
    {
        prepare, commit, execute
    };

    class pbft_operation
    {
    public:

        pbft_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash);

        /**
         * Store a session that waits on the result of the operation (will not persist across crashes)
         * @param session shared_ptr to session
         */
        virtual void set_session(std::shared_ptr<bzn::session_base> session);

        /**
         * @return the saved session, if any
         */
        virtual std::shared_ptr<bzn::session_base> session() const;

        /**
         * @return do we have a session
         */
        virtual bool has_session() const;

        /**
         * @return the operation_key_t that uniquely identifies this operation
         */
        virtual operation_key_t get_operation_key() const;

        virtual uint64_t get_sequence() const;
        virtual uint64_t get_view() const;
        virtual const hash_t& get_request_hash() const;

        /**
         * @return the current stage of the operation, defined as
         * pbft_operation_stage::prepare: waiting for preprepare and 2f+1 prepares
         * pbft_operation_stage::commit: prepared, waiting for 2f+1 commits
         * pbft_operation_stage::execute: committed-local, ready to be executed
         */
        virtual pbft_operation_stage get_stage() const = 0;

        /**
         * Save a pbft_message about this operation
         * @param msg preprepare/prepare/commit
         * @param encoded_msg the original message containing this message, signature intact.
         */
        virtual void record_pbft_msg(const pbft_msg& msg, const bzn_envelope& encoded_msg) = 0;

        /**
         * @return have we seen a preprepare for this operation
         */
        virtual bool is_preprepared() const = 0;

        /**
         * @return is this operation prepared (as defined in the pbft paper) at this node
         */
        virtual bool is_prepared() const = 0;

        /**
         * @return is this operation committed-local (as defined in the pbft paper) at this node
         */
        virtual bool is_committed() const = 0;

        /**
         * record the request that this operation is for. caller is responsible for checking that the request's hash
         * actually matches this operation's hash.
         * @param encoded_request original message containing the request, signature intact
         */
        virtual void record_request(const bzn_envelope& encoded_request) = 0;

        /**
         * advance the operation to the next stage, checking that doing so is legal
         * @param new_stage
         */
        virtual void advance_operation_stage(pbft_operation_stage new_stage) = 0;

        /**
         * @return do we know the full request associated with this operation?
         */
        virtual bool has_request() const = 0;

        /**
         * @return do we know the full request associated with this operation, and is it a database request?
         */
        virtual bool has_db_request() const = 0;

        /**
         * @return do we know the full request associated with this operation, and is it a new configuration?
         */
        virtual bool has_config_request() const = 0;

        /**
         * @return the signed envelope containing the request associated with this operation
         */
        virtual const bzn_envelope& get_request() const = 0;

        /**
         * @return the parsed new configuration associated with this operation
         */
        virtual const pbft_config_msg& get_config_request() const = 0;

        /**
         * @return the parsed database_msg associated wtih this operation
         */
        virtual const database_msg& get_database_msg() const = 0;

        /**
         * @return the recorded preprepare envelope
         */
        virtual bzn_envelope get_preprepare() const = 0;

        /**
         * @return the collection of saved prepare envelopes as a map of sender -> envelope
         */
        virtual std::map<uuid_t, bzn_envelope> get_prepares() const = 0;

        virtual ~pbft_operation() = default;

    private:
        bool session_saved = false;
        std::shared_ptr<bzn::session_base> listener_session;

        const uint64_t view;
        const uint64_t sequence;
        const bzn::hash_t request_hash;
    };
}
