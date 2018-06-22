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

#include <audit/audit.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/format.hpp>

using namespace bzn;

audit::audit(std::shared_ptr<bzn::node_base> node): node(node)
{

}

const std::list<std::string>&
audit::error_strings() const
{
    return this->recorded_errors;
}

uint
audit::error_count() const
{
    return this->recorded_errors.size();
}

void
audit::start()
{
    this->node->register_for_message("audit", std::bind(&audit::handle
                                                        , shared_from_this()
                                                        , std::placeholders::_1
                                                        , std::placeholders::_2));
    LOG(info) << "Audit module running";
}

void
audit::handle(const bzn::message& json, std::shared_ptr<bzn::session_base> /*session*/)
{
    audit_message message;
    message.ParseFromString(boost::beast::detail::base64_decode(json["audit-data"].asString()));

    LOG(debug) << "Got audit message" << message.DebugString();

    if(message.has_commit())
    {
        this->handle_commit(message.commit());
    }
    else if(message.has_leader_status())
    {
        this->handle_leader_status(message.leader_status());
    }
    else
    {
        LOG(error) << "Got an empty audit message?";
    }
}

void
audit::handle_leader_status(const leader_status& leader_status)
{
    if(this->recorded_leaders.count(leader_status.term()) == 0)
    {
        LOG(info) << "audit recording that leader of term " << leader_status.term() << " is " << leader_status.leader();
        this->recorded_leaders[leader_status.term()] = leader_status.leader();
    }
    else if(this->recorded_leaders[leader_status.term()] != leader_status.leader())
    {
        std::string err = str(boost::format(
                "Conflicting leader elected! %1% is the recorded leader of term %2%, but %3% claims to be the leader of the same term.")
                                             % this->recorded_leaders[leader_status.term()]
                                             % leader_status.term()
                                             % leader_status.leader());
        this->recorded_errors.push_back(err);
        LOG(fatal) << err;
    }
}

void
audit::handle_commit(const commit_notification& commit)
{
    if(this->recorded_commits.count(commit.log_index()) == 0)
    {
        LOG(info) << "audit recording that message " << commit.operation() << " is committed at index " << commit.log_index();
        this->recorded_commits[commit.log_index()] = commit.operation();
    }
    else if(this->recorded_commits[commit.log_index()] != commit.operation())
    {
        std::string err = str(boost::format(
                "Conflicting commit detected! %1% is the recorded entry at index %2%, but %3% has been committed with the same index.")
                              % this->recorded_commits[commit.log_index()]
                              % commit.log_index()
                              % commit.operation());
        this->recorded_errors.push_back(err);
        LOG(fatal) << err;
    }
}
