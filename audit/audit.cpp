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
#include <boost/asio/ip/udp.hpp>
#include <boost/format.hpp>
#include <boost/system/error_code.hpp>
#include <utils/bytes_to_debug_string.hpp>

using namespace bzn;

audit::audit(std::shared_ptr<bzn::asio::io_context_base> io_context
        , std::shared_ptr<bzn::node_base> node
        , size_t mem_size
        , std::shared_ptr<bzn::monitor_base> monitor
)

        : node(std::move(node))
        , io_context(std::move(io_context))
        , primary_alive_timer(this->io_context->make_unique_steady_timer())
        , mem_size(mem_size)
        , monitor(std::move(monitor))
{
}

void
audit::start()
{
    std::call_once(this->start_once, [this]()
    {
        this->node->register_for_message(bzn_envelope::kAudit,
            std::bind(&audit::handle, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

        LOG(debug) << "starting primary alive timer";
        this->reset_primary_alive_timer();
    });
}

void
audit::reset_primary_alive_timer()
{
    this->primary_dead_count = 0;

    this->primary_alive_timer->cancel();
    this->primary_alive_timer->expires_from_now(this->primary_timeout);
    this->primary_alive_timer->async_wait(std::bind(&audit::handle_primary_alive_timeout, shared_from_this(), std::placeholders::_1));
}

void
audit::handle_primary_alive_timeout(const boost::system::error_code& ec)
{
    if (ec && ec != boost::system::errc::operation_canceled)
    {
        LOG(trace) << "primary alive timeout canceled " << ec.message();
        return;
    }
    if (ec == boost::system::errc::operation_canceled)
    {
        LOG(trace) << "primary still alive";
        return;
    }

    this->monitor->send_counter(bzn::statistic::pbft_no_primary);

    this->primary_alive_timer->expires_from_now(this->primary_timeout);
    this->primary_alive_timer->async_wait(std::bind(&audit::handle_primary_alive_timeout, shared_from_this(), std::placeholders::_1));
}

void
audit::handle(const bzn_envelope& env, std::shared_ptr<bzn::session_base> /*session*/)
{
    audit_message message;
    if (!message.ParseFromString(env.audit()))
    {
        LOG(error) << "failed to parse audit message from " << env.sender();
    }

    LOG(trace) << "got audit message" << message.DebugString();

    if (message.has_pbft_commit())
    {
        this->handle_pbft_commit(message.pbft_commit());
    }
    else if (message.has_primary_status())
    {
        this->handle_primary_status(message.primary_status());
    }
    else if (message.has_failure_detected())
    {
        this->handle_failure_detected(message.failure_detected());
    }
    else
    {
        LOG(error) << "got an unknown audit message? " << message.DebugString();
    }
}

void audit::handle_primary_status(const primary_status& primary_status)
{
    std::lock_guard<std::mutex> lock(this->audit_lock);

    if (this->recorded_primaries.count(primary_status.view()) == 0)
    {
        LOG(info) << "observed primary of view " << primary_status.view() << " to be '" << primary_status.primary() << "'";
        this->monitor->send_counter(bzn::statistic::pbft_primary_alive);
        this->recorded_primaries[primary_status.view()] = primary_status.primary();
        this->trim();
    }
    else if (this->recorded_primaries[primary_status.view()] != primary_status.primary())
    {
        std::string err = str(boost::format(
                "Conflicting primary detected! '%1%' is the recorded primary of view %2%, but '%3%' claims to be the primary of the same view.")
                              % this->recorded_primaries[primary_status.view()]
                              % primary_status.view()
                              % primary_status.primary());
        this->monitor->send_counter(bzn::statistic::pbft_primary_conflict);
    }

    this->reset_primary_alive_timer();
}

void
audit::handle_pbft_commit(const pbft_commit_notification& commit)
{
    std::lock_guard<std::mutex> lock(this->audit_lock);

    this->monitor->send_counter(bzn::statistic::pbft_commit);

    if (this->recorded_pbft_commits.count(commit.sequence_number()) == 0)
    {
        LOG(debug) << "observed that message '" << bytes_to_debug_string(commit.operation()) << "' is committed at sequence " << commit.sequence_number();
        this->recorded_pbft_commits[commit.sequence_number()] = commit.operation();
        this->trim();
    }
    else if (this->recorded_pbft_commits[commit.sequence_number()] != commit.operation())
    {
        std::string err = str(boost::format(
                "Conflicting commit detected! '%1%' is the recorded entry at sequence %2%, but '%3%' has been committed with the same sequence.")
                              % this->recorded_pbft_commits[commit.sequence_number()]
                              % commit.sequence_number()
                              % commit.operation());
        this->monitor->send_counter(bzn::statistic::pbft_commit_conflict);
    }
}

void
audit::handle_failure_detected(const failure_detected& /*failure*/)
{
    std::lock_guard<std::mutex> lock(this->audit_lock);

    this->monitor->send_counter(bzn::statistic::pbft_failure_detected);
}

size_t
audit::current_memory_size()
{
    return this->recorded_pbft_commits.size() + this->recorded_primaries.size();
}

void
audit::trim()
{
    // Here we're removing the lowest term/log index entries, which is sort of like the oldest entries. I'd rather
    // remove entries at random, but that's not straightforward to do with STL containers without making some onerous
    // performance compromise.

    while(this->recorded_primaries.size() > this->mem_size)
    {
        this->recorded_primaries.erase(this->recorded_primaries.begin());
    }

    while(this->recorded_pbft_commits.size() > this->mem_size)
    {
        this->recorded_pbft_commits.erase(this->recorded_pbft_commits.begin());
    }
}
