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

#include <audit/audit_base.hpp>
#include <node/node_base.hpp>
#include <proto/bluzelle.pb.h>
#include <mutex>

namespace bzn
{

    class audit : public audit_base, public std::enable_shared_from_this<audit>
    {
    public:
        audit(std::shared_ptr<bzn::node_base> node);

        size_t error_count() const override;

        const std::list<std::string>& error_strings() const override;

        void handle(const bzn::message& message, std::shared_ptr<bzn::session_base> session) override;
        void handle_commit(const commit_notification&);
        void handle_leader_status(const leader_status&);

        void start() override;

    private:

        std::list<std::string> recorded_errors;
        const std::shared_ptr<bzn::node_base> node;

        std::map<uint64_t, bzn::uuid_t> recorded_leaders;
        std::map<uint64_t, std::string> recorded_commits;

        std::once_flag start_once;
    };

}