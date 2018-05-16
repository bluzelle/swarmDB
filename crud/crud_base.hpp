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

namespace bzn
{
    const std::string MSG_INVALID_RAFT_STATE = "INVALID_RAFT_STATE";
    const std::string MSG_INVALID_CRUD_COMMAND = "INVALID_CRUD";
    const std::string MSG_ELECTION_IN_PROGRESS = "ELECTION_IN_PROGRESS";
    const std::string MSG_VALUE_DOES_NOT_EXIST = "VALUE_DOES_NOT_EXIST";
    const std::string MSG_RECORD_EXISTS = "RECORD_EXISTS";
    const std::string MSG_NOT_THE_LEADER = "NOT_THE_LEADER";
    const std::string MSG_RECORD_NOT_FOUND = "RECORD_NOT_FOUND";
    const std::string MSG_INVALID_ARGUMENTS = "INVALID_ARGUMENTS";
    const std::string MSG_VALUE_SIZE_TOO_LARGE = "VALUE_SIZE_TOO_LARGE";
    const std::string MSG_COMMAND_NOT_SET = "COMMAND_NOT_SET";

    const std::string MSG_CMD_CREATE{"create"};
    const std::string MSG_CMD_READ{"read"};
    const std::string MSG_CMD_UPDATE{"update"};
    const std::string MSG_CMD_DELETE{"delete"};

    const std::string MSG_CMD_KEYS{"keys"};
    const std::string MSG_CMD_HAS{"has"};
    const std::string MSG_CMD_SIZE{"size"};

    class crud_base
    {
    public:
        virtual ~crud_base() = default;

        virtual void start() = 0;
    };

} // namespace bzn
