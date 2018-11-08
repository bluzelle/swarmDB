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
    if (auto result = this->unstable_storage->create(this->uuid, std::to_string(op->sequence), op->get_request().SerializeAsString());
        result != bzn::storage_base::result::ok)
    {
        LOG(fatal) << "failed to store pbft request: " << op->get_request().DebugString() << ", " << uint32_t(result);

        // these are fatal... something bad is going on.
        throw std::runtime_error("Failed to store pbft request! (" + std::to_string(uint8_t(result)) + ")");
    }

    // store requester session for eventual response...
    this->sessions_awaiting_response[op->sequence] = op->session();

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

        pbft_request request;

        if (!request.ParseFromString(*result))
        {
            // these are fatal... something bad is going on.
            throw std::runtime_error("Failed to create pbft_request from database read!");
        }

        LOG(info) << "Executing request " << request.DebugString() << "..., sequence: " << key;

        if (auto session_it = this->sessions_awaiting_response.find(this->next_request_sequence); session_it != this->sessions_awaiting_response.end())
        {
            // session found, but is the connection still around?
            auto session = session_it->second.lock();

            this->crud->handle_request(request.operation(), (session) ? session : nullptr);
        }
        else
        {
            // session not found then this was probably loaded from the database...
            this->crud->handle_request(request.operation(), nullptr);
        }

        this->io_context->post(std::bind(this->execute_handler, nullptr)); // TODO: need to find the pbft_operation here; requires pbft_operation not being an in-memory construct

        if (auto result = this->unstable_storage->remove(this->uuid, key); result != bzn::storage_base::result::ok)
        {
            // these are fatal... something bad is going on.
            throw std::runtime_error("Failed to remove pbft_request from database! (" + std::to_string(uint8_t(result)) + ")");
        }

        ++this->next_request_sequence;

        this->save_next_request_sequence();
    }
}


void
database_pbft_service::query(const pbft_request& request, uint64_t sequence_number) const
{
    // TODO: not sure how this works...

    LOG(info) << "Querying " << request.ShortDebugString()
              << " against ver " << std::min(sequence_number, this->next_request_sequence - 1);
}


bzn::hash_t
database_pbft_service::service_state_hash(uint64_t /*sequence_number*/) const
{
    // TODO: not sure how this works...

    return "";
}

bzn::service_state_t
database_pbft_service::get_service_state(uint64_t sequence_number) const
{
    // retrieve database state at this sequence/checkpoint
    /*
        return this->crud->get_state(sequence_number);
    */

    return std::string("state_") + std::to_string(sequence_number);
}

bool
database_pbft_service::set_service_state(uint64_t sequence_number, const bzn::service_state_t& /*data*/)
{
    // initialize database state from checkpoint data
    /*
        if (!this->crud->set_state(sequence_number, data))
            return false;
    */

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
database_pbft_service::consolidate_log(uint64_t sequence_number)
{
    LOG(info) << "TODO: consolidating log at sequence number " << sequence_number;

    // tell the database to set a checkpoint
    /*
        this->crud->remember_state(sequence_number);
    */
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
        std::to_string(this->next_request_sequence)); result != bzn::storage_base::result::ok)
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
        std::to_string(this->next_request_sequence)); result != bzn::storage_base::result::ok)
    {
        LOG(error) << "failed to save next_request_sequence: " << uint32_t(result);
        return;
    }

    LOG(debug) << "updated: next_request_sequence: " << this->next_request_sequence;
}
