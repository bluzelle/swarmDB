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

#include "pbft/dummy_pbft_service.hpp"

using namespace bzn;

dummy_pbft_service::dummy_pbft_service(std::shared_ptr<bzn::asio::io_context_base> io_context) : io_context(std::move(io_context))
{

}

void
dummy_pbft_service::apply_operation(const pbft_request& request, uint64_t sequence_number)
{
    std::lock_guard<std::mutex> lock(this->lock);

    this->waiting_requests[sequence_number] = request;

    while (this->waiting_requests.count(this->next_request_sequence) > 0)
    {
        auto req = waiting_requests[this->next_request_sequence];

        LOG(info) << "Executing request " << req.ShortDebugString() << ", sequence " << this->next_request_sequence
                  << "\n";


        boost::asio::post(std::bind(this->execute_handler, req, this->next_request_sequence));

        this->waiting_requests.erase(this->next_request_sequence);
        this->next_request_sequence++;
    }
}

void
dummy_pbft_service::query(const pbft_request& request, uint64_t sequence_number) const
{
    LOG(info) << "Querying " << request.ShortDebugString()
              << " against ver " << std::min(sequence_number, this->next_request_sequence - 1);
}

void
dummy_pbft_service::consolidate_log(uint64_t sequence_number)
{
    LOG(info) << "Consolidating log at sequence number " << sequence_number;
}


uint64_t
dummy_pbft_service::applied_requests_count()
{
    return this->next_request_sequence - 1;
}

void
dummy_pbft_service::register_execute_handler(std::function<void(const pbft_request&, uint64_t)> handler)
{
    this->execute_handler = std::move(handler);
}

bzn::hash_t
dummy_pbft_service::service_state_hash(uint64_t sequence_number) const
{
    return "I don't actually have a database [" + std::to_string(sequence_number) + "]";
}
