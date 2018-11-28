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


dummy_pbft_service::dummy_pbft_service(std::shared_ptr<bzn::asio::io_context_base> io_context)
    : io_context(std::move(io_context))
{

}

void
dummy_pbft_service::apply_operation(const std::shared_ptr<pbft_operation>& op)
{
    std::lock_guard<std::mutex> lock(this->lock);

    this->waiting_operations[op->sequence] = std::move(op);

    while (this->waiting_operations.count(this->next_request_sequence) > 0)
    {
        auto op = waiting_operations[this->next_request_sequence];

        LOG(info) << "Executing request " << op->debug_string() << ", sequence " << this->next_request_sequence
                  << "\n";

        this->send_execute_response(op);

        // todo: use shared from this as post could act on a long gone dummy_pbft_service?
        this->io_context->post(std::bind(this->execute_handler, op));

        this->waiting_operations.erase(this->next_request_sequence);
        this->next_request_sequence++;
    }
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
dummy_pbft_service::register_execute_handler(execute_handler_t handler)
{
    this->execute_handler = std::move(handler);
}

bzn::hash_t
dummy_pbft_service::service_state_hash(uint64_t sequence_number) const
{
    return "I don't actually have a database [" + std::to_string(sequence_number) + "]";
}

std::shared_ptr<bzn::service_state_t>
dummy_pbft_service::get_service_state(uint64_t sequence_number) const
{
    return std::make_shared<std::string>("I don't actually have a database [" + std::to_string(sequence_number) + "]");
}

bool
dummy_pbft_service::set_service_state(uint64_t /*sequence_number*/, const bzn::service_state_t& /*data*/)
{
    return true;
}

void
dummy_pbft_service::save_service_state_at(uint64_t /*sequence_number*/)
{
}

void
dummy_pbft_service::send_execute_response(const std::shared_ptr<pbft_operation>& op)
{
    database_response resp;
    resp.mutable_read()->set_value("dummy database execution of " + op->debug_string());

    LOG(debug) << "Sending request result " << resp.ShortDebugString();
    op->session()->send_datagram(std::make_shared<std::string>(resp.SerializeAsString()));
}
