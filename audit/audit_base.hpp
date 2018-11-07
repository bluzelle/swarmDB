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
    const std::string LEADER_CONFLICT_METRIC_NAME = "raft.safety.leader_conflict";
    const std::string RAFT_COMMIT_CONFLICT_METRIC_NAME = "raft.safety.commit_conflict";
    const std::string LEADER_STUCK_METRIC_NAME = "raft.liveness.leader_stuck";
    const std::string NO_LEADER_METRIC_NAME = "raft.liveness.no_leader";
    const std::string RAFT_COMMIT_METRIC_NAME = "raft.stats.commit_heard";
    const std::string NEW_LEADER_METRIC_NAME = "raft.stats.new_leader_heard";

    const std::string PBFT_COMMIT_METRIC_NAME = "pbft.stats.commit_heard";
    const std::string PRIMARY_HEARD_METRIC_NAME = "pbft.stats.primary_heard";
    const std::string FAILURE_DETECTED_METRIC_NAME = "pbft.stats.failure_detected";
    const std::string PBFT_COMMIT_CONFLICT_METRIC_NAME = "pbft.safety.commit_conflict";
    const std::string PRIMARY_CONFLICT_METRIC_NAME = "pbft.safety.primary_conflict";
    const std::string NO_PRIMARY_METRIC_NAME = "pbft.liveness.no_primary";

    const std::string STATSD_COUNTER_FORMAT = ":1|c";

    class audit_base
    {
    public:

        virtual ~audit_base() = default;

        virtual void start() = 0;

        virtual size_t error_count() const = 0;

        virtual const std::list<std::string> &error_strings() const = 0;

        virtual void handle(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session) = 0;

        virtual void handle_raft_commit(const raft_commit_notification&) = 0;

        virtual void handle_leader_status(const leader_status&) = 0;

        virtual void handle_pbft_commit(const pbft_commit_notification&) = 0;

        virtual void handle_primary_status(const primary_status&) = 0;

        virtual void handle_failure_detected(const failure_detected&) = 0;
    };

}
