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

#include <include/bluzelle.hpp>
#include <node/session_base.hpp>
#include <proto/database.pb.h>


namespace bzn
{
    const std::string MSG_INVALID_UUID  = "INVALID_UUID";
    const std::string MSG_INVALID_KEY   = "INVALID_KEY";
    const std::string MSG_INVALID_SUB   = "INVALID_SUB";
    const std::string MSG_DUPLICATE_SUB = "DUPLICATE_SUB";

    class subscription_manager_base
    {
    public:
        virtual ~subscription_manager_base() = default;

        virtual void start() = 0;

        virtual void   subscribe(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session) = 0;

        virtual void unsubscribe(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session) = 0;

        virtual void inspect_commit(const database_msg& msg) = 0;

    };

} // namespace bzn
