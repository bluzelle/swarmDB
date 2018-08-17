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

    class pbft_service : public pbft_service_base
    {
    public:
        pbft_service(std::shared_ptr<pbft_failure_detector_base> failure_detector);

        void commit_operation(std::shared_ptr<pbft_operation> operation) override;

        const pbft_request& get_last_request();
        uint64_t applied_requests_count();

    private:
        uint64_t next_request_sequence = 1;
        std::unordered_map<uint64_t, std::shared_ptr<pbft_operation>> waiting_requests;
        pbft_request remembered_request;
        std::shared_ptr<pbft_failure_detector_base> failure_detector;

        std::mutex lock;
    };

}
