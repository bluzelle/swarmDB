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
#include <string>

namespace bzn
{
    enum class statistic
    {
        hash_computed,
        hash_computed_bytes,
        signature_computed,
        signature_computed_bytes,
        signature_verified,
        signature_verified_bytes,
        signature_rejected,

        session_opened,
        message_sent,
        message_sent_bytes,

        pbft_no_primary,
        pbft_primary_alive,
        pbft_failure_detected,
        pbft_commit_conflict,
        pbft_primary_conflict,

        pbft_commit,

        request_latency
    };

    class monitor_base
    {
    public:

        /*
         * Start timing for some statistic (e.g. a request completion). Will be ignored if this is a duplicate of some
         * other recent timer.
         * @instance_id some globally unique name for this instance of the timer (such as the request hash)
         */
        virtual void start_timer(std::string instance_id) = 0;

        /*
         * Finish timing for some statistic (e.g. a request completion). Will be ignored if no such timer has been
         * started.
         * @instance_id some globally unique name for this instance of the timer (such as the request hash)
         * @stat the statistic under which to send the measurement
         */
        virtual void finish_timer(statistic stat, std::string instance_id) = 0;

        /*
         * Send a counter statistic
         * @stat statistic to send
         * @amount amount of that statistic that has been observed
         */
        virtual void send_counter(statistic stat, uint64_t amount = 1) = 0;

        virtual ~monitor_base() = default;
    };
}
