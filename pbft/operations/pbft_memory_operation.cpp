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

#include <pbft/operations/pbft_memory_operation.hpp>
#include <boost/format.hpp>
#include <string>

using namespace bzn;

pbft_memory_operation::pbft_memory_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash, std::shared_ptr<peers_beacon_base> peers)
        : pbft_operation(view, sequence, request_hash)
        , peers(std::move(peers))
{
}

bool
pbft_memory_operation::has_request() const
{
    return this->request_saved;
}

bool
pbft_memory_operation::has_db_request() const
{
    return this->has_request() && this->request.payload_case() == bzn_envelope::kDatabaseMsg;
}

bool
pbft_memory_operation::has_config_request() const
{
    return this->has_request() && this->request.payload_case() == bzn_envelope::kPbftInternalRequest;
}

const bzn_envelope&
pbft_memory_operation::get_request() const
{
    if (!this->request_saved)
    {
        throw std::runtime_error("Tried to get a request that is not saved");
    }

    return this->request;
}

const pbft_config_msg&
pbft_memory_operation::get_config_request() const
{
    if (!this->has_config_request())
    {
        throw std::runtime_error("Tried to get a config request that is not saved");
    }

    return this->parsed_config;
}

const database_msg&
pbft_memory_operation::get_database_msg() const
{
    if (!this->has_db_request())
    {
        throw std::runtime_error("Tried to get a db request that is not saved");
    }

    return this->parsed_db;
}

void
pbft_memory_operation::record_request(const bzn_envelope& wrapped_request)
{

    if (wrapped_request.payload_case() == bzn_envelope::kDatabaseMsg)
    {
        if (!this->parsed_db.ParseFromString(wrapped_request.database_msg()))
        {
            LOG(error) << "Failed to parse database request";
            LOG(error) << wrapped_request.database_msg();
            return;
        }
    }
    else if (wrapped_request.payload_case() == bzn_envelope::kPbftInternalRequest)
    {
        if (!this->parsed_config.ParseFromString(wrapped_request.pbft_internal_request()))
        {
            LOG(error) << "Failed to parse pbft internal request";
            return;
        }
    }
    else
    {
        LOG(error) << "Tried to record request as envelope that does not actually contain a request type";
        return;
    }

    this->request = wrapped_request;
    this->request_saved = true;

}

void pbft_memory_operation::record_pbft_msg(const pbft_msg& message, const bzn_envelope& original_message)
{
    switch(message.type())
    {
        case pbft_msg_type::PBFT_MSG_PREPREPARE :
            this->preprepare_seen = true;
            this->preprepare_message = original_message;
            break;
        case pbft_msg_type::PBFT_MSG_PREPARE :
            this->prepares_seen.emplace(original_message.sender());
            this->prepare_messages[original_message.sender()] = original_message;
            break;
        case pbft_msg_type::PBFT_MSG_COMMIT :
            this->commits_seen.emplace(original_message.sender());
            break;
        default:
            throw std::runtime_error("this is not an appropriate pbft_msg_type");
    }

    //TODO: save original message
}

bool
pbft_memory_operation::is_preprepared() const
{
    return this->preprepare_seen;
}

bool
pbft_memory_operation::is_prepared() const
{
    throw std::runtime_error("TODO");
    //return this->has_request() && this->is_preprepared() && this->prepares_seen.size() > 2 * this->faulty_nodes_bound();
}

bool
pbft_memory_operation::is_committed() const
{
    throw std::runtime_error("TODO");
    //return this->is_prepared() && this->commits_seen.size() > 2 * this->faulty_nodes_bound();
}

void
pbft_memory_operation::advance_operation_stage(bzn::pbft_operation_stage new_stage)
{
    switch(new_stage)
    {
        case pbft_operation_stage::prepare :
            throw std::runtime_error("cannot advance to initial stage");
        case pbft_operation_stage::commit :
            if (!this->is_preprepared() || this->stage != pbft_operation_stage::prepare)
            {
                throw std::runtime_error("illegal move to commit phase");
            }
            break;
        case pbft_operation_stage::execute :
            if (!this->is_committed() || this->stage != pbft_operation_stage::commit)
            {
                throw std::runtime_error("illegal move to execute phase");
            }
            break;
        default:
            throw std::runtime_error("unknown pbft_operation_stage");
    }

    this->stage = new_stage;
}

pbft_operation_stage
pbft_memory_operation::get_stage() const
{
    return this->stage;
}

bzn_envelope
pbft_memory_operation::get_preprepare() const
{
    return this->preprepare_message;
}

std::map<bzn::uuid_t, bzn_envelope>
pbft_memory_operation::get_prepares() const
{
    return this->prepare_messages;
}
