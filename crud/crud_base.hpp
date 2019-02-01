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
#include <proto/bluzelle.pb.h>


namespace bzn
{
    class crud_base
    {
    public:
        virtual ~crud_base() = default;

        virtual void handle_request(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session) = 0;

        virtual void start() = 0;

        virtual bool save_state() = 0;

        virtual std::shared_ptr<std::string> get_saved_state() = 0;

        virtual bool load_state(const std::string& state) = 0;
    };

} // namespace bzn
