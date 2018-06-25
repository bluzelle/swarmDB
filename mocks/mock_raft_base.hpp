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

#include <raft/raft_base.hpp>
#include <gmock/gmock.h>

namespace bzn
{
    class Mockraft_base : public raft_base {
    public:
        MOCK_METHOD0(get_state,
                     bzn::raft_state());
        MOCK_METHOD0(start,
                     void());
        MOCK_METHOD0(get_leader,
                     bzn::peer_address_t());
        MOCK_METHOD2(append_log,
                     bool(const bzn::message& msg, const bzn::log_entry_type entry_type));
        MOCK_METHOD1(register_commit_handler,
                     void(bzn::raft_base::commit_handler handler));
    };

} // namespace bzn