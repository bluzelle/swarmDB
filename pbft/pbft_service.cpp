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

#include "pbft/pbft_service.hpp"

using namespace bzn;

void
pbft_service::commit_request(uint64_t sequence, const pbft_request& request)
{
    waiting_requests[sequence] = request;

    while (waiting_requests.count(this->next_request_sequence) > 0)
    {
        const pbft_request& req = waiting_requests[sequence];

        LOG(info) << "Executing request " << req.ShortDebugString() << ", sequence " << this->next_request_sequence
                  << "\n";
        this->remembered_request = req;

        waiting_requests.erase(this->next_request_sequence);
        this->next_request_sequence++;
    }
}

uint64_t
pbft_service::applied_requests_count()
{
    return this->next_request_sequence - 1;
}

const pbft_request&
pbft_service::get_last_request()
{
    return this->remembered_request;
}
