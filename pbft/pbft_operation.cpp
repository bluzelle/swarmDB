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

#include "pbft_operation.hpp"
#include <boost/format.hpp>
#include <string>

using namespace bzn;

pbft_operation::pbft_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash, std::shared_ptr<const std::vector<peer_address_t>> peers)
        : view(view)
          , sequence(sequence)
          , request_hash(request_hash)
          , peers(std::move(peers))
{
}

bool
pbft_operation::has_request() const
{
    return this->request_saved;
}

const pbft_request&
pbft_operation::get_request() const
{
    if (!this->request_saved)
    {
        throw std::runtime_error("Tried to get a request that is not saved");
    }

    return this->parsed_request;
}

const bzn::encoded_message&
pbft_operation::get_encoded_request() const
{
    if (!this->request_saved)
    {
        throw std::runtime_error("Tried to get a request that is not saved");
    }

    return this->encoded_request;
}

void
pbft_operation::record_request(const bzn::encoded_message& wrapped_request)
{
    // not actually parsing the request because, for now, its still json
    this->encoded_request = wrapped_request;

    if (this->parsed_request.ParseFromString(wrapped_request))
    {
        LOG(error) << "Tried to record request as something not a valid request (perhaps an old json request?)";
        // We don't consider this a failure case, for now, because it could be an old json request
    }

    this->request_saved = true;
}

void
pbft_operation::record_preprepare(const bzn_envelope& /*encoded_preprepare*/)
{
    this->preprepare_seen = true;
}

bool
pbft_operation::has_preprepare() const
{
    return this->preprepare_seen;
}

void
pbft_operation::record_prepare(const bzn_envelope& encoded_prepare)
{
    // TODO: Save message
    this->prepares_seen.insert(encoded_prepare.sender());
}

size_t
pbft_operation::faulty_nodes_bound() const
{
    return (this->peers->size() - 1) / 3;
}

bool
pbft_operation::is_prepared() const
{
    return this->has_request() && this->has_preprepare() && this->prepares_seen.size() > 2 * this->faulty_nodes_bound();
}

void
pbft_operation::record_commit(const bzn_envelope& encoded_commit)
{
    this->commits_seen.insert(encoded_commit.sender());
}

bool
pbft_operation::is_committed() const
{
    return this->is_prepared() && this->commits_seen.size() > 2 * this->faulty_nodes_bound();
}

void
pbft_operation::begin_commit_phase()
{
    if (!this->is_prepared() || this->state != pbft_operation_state::prepare)
    {
        throw std::runtime_error("Illegaly tried to move to commit phase");
    }

    this->state = pbft_operation_state::commit;
}

void
pbft_operation::end_commit_phase()
{
    if (!this->is_committed() || this->state != pbft_operation_state::commit)
    {
        throw std::runtime_error("Illegally tried to end the commit phase");
    }

    this->state = pbft_operation_state::committed;
}

operation_key_t
pbft_operation::get_operation_key() const
{
    return {this->view, this->sequence, this->request_hash};
}

pbft_operation_state
pbft_operation::get_state() const
{
    return this->state;
}

std::string
pbft_operation::debug_string() const
{
    return boost::str(boost::format("(v%1%, s%2%) - %3%") % this->view % this->sequence % this->parsed_request.ShortDebugString());
}

void
pbft_operation::set_session(std::weak_ptr<bzn::session_base> session)
{
    this->listener_session = std::move(session);
}

std::weak_ptr<bzn::session_base>
pbft_operation::session() const
{
    return this->listener_session;
}
