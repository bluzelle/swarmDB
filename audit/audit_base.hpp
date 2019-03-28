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
#include <proto/audit.pb.h>

namespace bzn
{
    class audit_base
    {
    public:

        virtual ~audit_base() = default;

        virtual void start() = 0;

        virtual void handle(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session) = 0;

        virtual void handle_pbft_commit(const pbft_commit_notification&) = 0;

        virtual void handle_primary_status(const primary_status&) = 0;

        virtual void handle_failure_detected(const failure_detected&) = 0;
    };

}
