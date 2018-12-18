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

#include <pbft/operations/pbft_operation.hpp>

using namespace bzn;

pbft_operation::pbft_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash)
        : view(view)
        , sequence(sequence)
        , request_hash(request_hash)
{
}

operation_key_t
pbft_operation::get_operation_key() const
{
    return {this->view, this->sequence, this->request_hash};
}

void
pbft_operation::set_session(std::shared_ptr<bzn::session_base> session)
{
    this->listener_session = std::move(session);
    this->session_saved = true;
}

std::shared_ptr<bzn::session_base>
pbft_operation::session() const
{
    return this->listener_session;
}

bool
pbft_operation::has_session() const
{
    return this->session_saved;
}

uint64_t
pbft_operation::get_sequence() const
{
    return this->sequence;
}

uint64_t
pbft_operation::get_view() const
{
    return this->view;
}

const hash_t&
pbft_operation::get_request_hash() const
{
    return this->request_hash;
}
