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


#include <pbft/database_pbft_service.hpp>
#include <boost/lexical_cast.hpp>


using namespace bzn;

namespace
{
    const std::string NEXT_REQUEST_SEQUENCE_KEY{"next_request_sequence"};
}


database_pbft_service::database_pbft_service(
    std::shared_ptr<bzn::asio::io_context_base> io_context,
    std::shared_ptr<bzn::storage_base> unstable_storage,
    std::shared_ptr<bzn::crud_base> crud,
    bzn::uuid_t uuid)
    : io_context(std::move(io_context))
    , unstable_storage(std::move(unstable_storage))
    , crud(std::move(crud))
    , uuid(std::move(uuid))
{
    this->load_next_request_sequence();
}

database_pbft_service::~database_pbft_service()
{
    this->save_next_request_sequence();
}


void
database_pbft_service::apply_operation(const std::shared_ptr<bzn::pbft_operation>& op)
{
    std::lock_guard<std::mutex> lock(this->lock);

    // store op...
    if (auto result = this->unstable_storage->create(this->uuid, std::to_string(op->sequence), op->get_database_msg().SerializeAsString());
        result != bzn::storage_result::ok)
    {
        LOG(fatal) << "failed to store pbft request: " << op->get_database_msg().DebugString() << ", " << uint32_t(result);

        // these are fatal... something bad is going on.
        throw std::runtime_error("Failed to store pbft request! (" + std::to_string(uint8_t(result)) + ")");
    }

    // store requester session for eventual response...
    this->operations_awaiting_result[op->sequence] = op;

    this->process_awaiting_operations();
}


void
database_pbft_service::process_awaiting_operations()
{
    while (this->unstable_storage->has(this->uuid, std::to_string(this->next_request_sequence)))
    {
        const key_t key{std::to_string(this->next_request_sequence)};

        auto result = this->unstable_storage->read(this->uuid, key);

        if (!result)
        {
            // these are fatal... something bad is going on.
            throw std::runtime_error("Failed to store pbft request!");
        }

        database_msg request;

        if (!request.ParseFromString(*result))
        {
            // these are fatal... something bad is going on.
            throw std::runtime_error("Failed to create pbft_request from database read!");
        }

        LOG(info) << "Executing request " << request.DebugString() << "..., sequence: " << key;

        auto op_it = this->operations_awaiting_result.find(this->next_request_sequence);

        if (op_it != this->operations_awaiting_result.end() && op_it->second->has_session())
        {

            // session found, but is the connection still around?
            auto session = op_it->second->session().lock();

            LOG(info) << "We do not have a session for the result of this request";
            this->crud->handle_request("caller id", request, (session) ? session : nullptr);
        }
        else
        {
            // session not found then this was probably loaded from the database...
            LOG(info) << "We do not have a session for the result of this request";
            this->crud->handle_request("caller_id", request, nullptr);
        }

        if (op_it != this->operations_awaiting_result.end())
        {
            this->io_context->post(std::bind(this->execute_handler, (*op_it).second));
        }

        if (auto result = this->unstable_storage->remove(this->uuid, key); result != bzn::storage_result::ok)
        {
            // these are fatal... something bad is going on.
            throw std::runtime_error("Failed to remove pbft_request from database! (" + std::to_string(uint8_t(result)) + ")");
        }

        if (this->next_request_sequence == this->next_checkpoint)
        {
            if (this->crud->save_state())
            {
                this->last_checkpoint = this->next_request_sequence;
            }
        }

        ++this->next_request_sequence;

        this->save_next_request_sequence();
    }
}

bzn::hash_t
database_pbft_service::service_state_hash(uint64_t /*sequence_number*/) const
{
    // TODO: not sure how this works...

    return "";
}

std::shared_ptr<bzn::service_state_t>
database_pbft_service::get_service_state(uint64_t sequence_number) const
{
    if (sequence_number == this->last_checkpoint)
    {
        return this->crud->get_saved_state();
    }

    return nullptr;
}

bool
database_pbft_service::set_service_state(uint64_t sequence_number, const bzn::service_state_t& data)
{
    // initialize database state from checkpoint data
    if (!this->crud->load_state(data))
    {
        return false;
    }
    this->last_checkpoint = sequence_number;

    // remove all backlogged requests prior to checkpoint
    uint64_t seq = this->next_request_sequence;
    while (seq <= sequence_number)
    {
        const key_t key{std::to_string(seq)};
        this->unstable_storage->remove(uuid, key);
        seq++;
    }

    this->next_request_sequence = seq;
    this->process_awaiting_operations();
    return true;
}

void
database_pbft_service::save_service_state_at(uint64_t sequence_number)
{
    this->next_checkpoint = sequence_number;
}

void
database_pbft_service::consolidate_log(uint64_t sequence_number)
{
    LOG(info) << "TODO: consolidating log at sequence number " << sequence_number;
}

void
database_pbft_service::register_execute_handler(bzn::execute_handler_t handler)
{
    this->execute_handler = std::move(handler);
}


uint64_t
database_pbft_service::applied_requests_count() const
{
    return this->next_request_sequence - 1;
}


void
database_pbft_service::load_next_request_sequence()
{
    if (auto result = this->unstable_storage->read(this->uuid, NEXT_REQUEST_SEQUENCE_KEY); result)
    {
        LOG(debug) << "read: next_request_sequence: " << *result;

        this->next_request_sequence = boost::lexical_cast<uint64_t>(*result);
        return;
    }

    if (auto result = this->unstable_storage->create(this->uuid, NEXT_REQUEST_SEQUENCE_KEY,
        std::to_string(this->next_request_sequence)); result != bzn::storage_result::ok)
    {
        LOG(fatal) << "failed to create \"" << NEXT_REQUEST_SEQUENCE_KEY << "\": " << uint32_t(result);

        throw std::runtime_error("Failed to create: " + NEXT_REQUEST_SEQUENCE_KEY);
    }

    LOG(debug) << "next_request_sequence: " << this->next_request_sequence;
}


void
database_pbft_service::save_next_request_sequence()
{
    if (auto result = this->unstable_storage->update(this->uuid, NEXT_REQUEST_SEQUENCE_KEY,
        std::to_string(this->next_request_sequence)); result != bzn::storage_result::ok)
    {
        LOG(error) << "failed to save next_request_sequence: " << uint32_t(result);
        return;
    }

    LOG(debug) << "updated: next_request_sequence: " << this->next_request_sequence;
}
