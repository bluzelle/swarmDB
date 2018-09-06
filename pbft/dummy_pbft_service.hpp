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

#include <pbft/pbft_service_base.hpp>
#include <pbft/pbft_failure_detector_base.hpp>
#include <unordered_map>

namespace bzn
{
    // This is a placeholder implementation! Eventually, a pbft_service will
    // wrap crud. It needs to be that way around (rather than crud calling
    // pbft) because pbft needs to decide when crud gets to see messages.

    class dummy_pbft_service : public pbft_service_base
    {
    public:
        dummy_pbft_service(std::shared_ptr<pbft_failure_detector_base> failure_detector);

        void apply_operation(std::shared_ptr<pbft_operation> op) override;
        void query(const pbft_request& request, uint64_t sequence_number) const override;
        void consolidate_log(uint64_t sequence_number) override;

        uint64_t applied_requests_count();

    private:
        void send_execute_response(const std::shared_ptr<pbft_operation>& op);

        uint64_t next_request_sequence = 1;
        std::unordered_map<uint64_t, std::shared_ptr<pbft_operation>> waiting_operations;
        std::shared_ptr<pbft_failure_detector_base> failure_detector;

        std::mutex lock;
    };

}
