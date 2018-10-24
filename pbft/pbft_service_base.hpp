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

#include <cstdint>
#include <include/bluzelle.hpp>
#include <proto/bluzelle.pb.h>
#include <pbft/pbft_operation.hpp>

namespace bzn
{
    using execute_handler_t = std::function<void(const pbft_request&, uint64_t)>;

    class pbft_service_base
    {
    public:

        virtual ~pbft_service_base() = default;

        /*
         * Interface for some service that is replicated by pbft. There could a layer that just handles the logic
         * required by this interface, but if the underlying service is a database than that would probably be awkward.
         *
         * Caller must guarantee:
         * - If apply_operation(x, y) is called, then apply_operation(x2, y) will never be called with x != x2
         * - If apply_operation(_, y) is called and y != 0, then apply_operation(_, y-1) will be called at least once
         *     (may be before or after apply_operation(_, y).
         * - If consolidate_log(y) is called, then no subsequenent call to query(x, y2) will have y2 < y
         *     (so the service may consolidate all the updates before y into a single version of the service on disk)
         * - consolidate_log(y) is called if forall y2<y, apply_operation(_, y2) has already been called
         * - After consolidate_log(y) is called, no future call apply_operation(_, y2) or query(_, y2)
         *     or consolidate_log(y2) will have y2 < y
         *
         * Implementation must guarantee:
         * - If apply_operation(x, y) is called with y != 0, the request will not be executed until after the request
         *     supplied in the call apply_operation(x2, y-1).
         * - When apply_operation(x, y) is called, it will be persisted to disk before the method returns even if
         *     x cannot yet be executed due to the first constraint
         * - The result of query(x, y) is the result of applying x against the version of the service where every
         *   operation with sequence number <= y has been applied, except operations that cannot yet be applied
         *   due to the first constraint. So, where cp is the last call to consolidate_log(cp) and curr is the last
         *   executed request sequence number, when query(x, y) is called, assuming the service is crud:
         *      if y >= curr, use the most up to date version of the key you have applied (still obeying constraint 1)
         *      if curr > y > cp, use the most up to date version of the key written before y, or the stable checkpoint
         *        if it hasn't been written to in that interval
         *      if cp == y, use the version of the key from the stable checkpoint
         *      if cp > y, the caller has broken a constraint
         * - Operations are applied at most once (crud operations are idempotent anyway)
         *
         * Notably not guarenteed:
         *  - apply_operation(x, y) called only once for the same value y
         *  - apply_operation(x, y) called with monotomically increasing y
         */

        /*
         * PBFT has concluded that an operation is committed-local, it can now be applied as soon as all earlier
         * operations have been applied.
         */
        virtual void apply_operation(const std::shared_ptr<pbft_operation>& op) = 0;

        /*
         * Apply some read-only operation to the history of the service at some particular sequence number (either the
         * sequence number is >= any the service has seen before because we want the most recent version, or we are
         * querying some stable checkpoint to introduce a new node).
         */
        virtual void query(const pbft_request& request, uint64_t sequence_number) const = 0;

        /*
         * Get the hash of the database state (presumably this will be a merkle tree root, but the details don't matter
         * for now)- same semantics as query
         */
        virtual bzn::hash_t service_state_hash(uint64_t sequence_number) const = 0;

        /*
         * A checkpoint has been stabilized, so we no longer need any history from before then.
         */
        virtual void consolidate_log(uint64_t sequence_number) = 0;

        /*
         * Callback when a request is executed (not committed, since the service is responsible for the difference
         * between the two). Should only be called once for each sequence number, in strictly increasing order.
         */
        virtual void register_execute_handler(bzn::execute_handler_t handler) = 0;

    };

}
