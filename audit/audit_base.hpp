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

#include <string>
#include <list>
#include <memory>

#include <include/bluzelle.hpp>
#include <node/node_base.hpp>
#include <node/session_base.hpp>
#include <proto/bluzelle.pb.h>

namespace bzn
{

    class audit_base
    {
        virtual void start() = 0;

        virtual size_t error_count() const = 0;

        virtual const std::list<std::string> &error_strings() const = 0;

        virtual void handle(const bzn::message& msg, std::shared_ptr<bzn::session_base> session) = 0;

        virtual void handle_commit(const commit_notification&) = 0;

        virtual void handle_leader_status(const leader_status&) = 0;
    };

}
