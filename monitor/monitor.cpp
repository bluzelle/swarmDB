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

#include <monitor/monitor.hpp>
#include <boost/format.hpp>

using namespace bzn;

namespace
{
    std::unordered_map<statistic, std::string> statistic_names{
            {
                    {statistic::hash_computed, "crypto.hashes_computed"},
                    {statistic::hash_computed_bytes, "crypto.bytes_hashed"},
                    {statistic::signature_computed, "crypto.signatures_computed"},
                    {statistic::signature_computed_bytes, "crypto.bytes_signed"},
                    {statistic::signature_verified, "crypto.signatures_verified"},
                    {statistic::signature_verified_bytes, "crypto.bytes_verified"},
                    {statistic::signature_rejected, "crypto.signatures_rejected"},

                    {statistic::session_opened, "node.sessions_opened"},
                    {statistic::message_sent, "node.messages_sent"},
                    {statistic::message_sent_bytes, "node.bytes_sent"},

                    {statistic::pbft_no_primary, "pbft.liveness.no_primary"},
                    {statistic::pbft_primary_alive, "pbft.liveness.primary_alive"},
                    {statistic::pbft_commit, "pbft.liveness.commit"},
                    {statistic::pbft_failure_detected, "pbft.liveness.failure_detected"},
                    {statistic::pbft_commit_conflict, "pbft.safety.commit_conflict"},
                    {statistic::pbft_primary_conflict, "pbft.safety.primary_conflict"},

                    {statistic::request_latency, "total-server-latency"}
            }
    };
}

monitor::monitor(std::shared_ptr<bzn::options_base> options, std::shared_ptr<bzn::asio::io_context_base> context, std::shared_ptr<bzn::system_clock_base> clock)
        : time_last_sent(clock->microseconds_since_epoch())
        , options(std::move(options))
        , context(std::move(context))
        , clock(std::move(clock))
        , socket(this->context->make_unique_udp_socket())
        , monitor_endpoint(this->options->get_monitor_endpoint(this->context))
        , scope_prefix("com.bluzelle.swarm.singleton.node." + this->options->get_uuid())
{
    if (this->monitor_endpoint)
    {
        LOG(info) << boost::format("Will send stats to %1%:%2%")
                     % this->monitor_endpoint->address().to_string()
                     % this->monitor_endpoint->port();
    }
    else
    {
        LOG(info) << "No monitor is configured; stats will not be collected";
    }

}

void
monitor::start_timer(std::string timer_id)
{
    if (!this->monitor_endpoint)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(this->lock);
    if (this->start_times.find(timer_id) != this->start_times.end())
    {
        return;
    }

    this->start_times.emplace(std::make_pair(timer_id, this->clock->microseconds_since_epoch()));
    this->ordered_timers.push_back(timer_id);

    while (this->ordered_timers.size() > this->options->get_simple_options().get<uint64_t>(bzn::option_names::MONITOR_MAX_TIMERS))
    {
        this->start_times.erase(this->ordered_timers.front());
        this->ordered_timers.pop_front();
    }
}

void
monitor::finish_timer(bzn::statistic stat, std::string timer_id)
{
    if (!this->monitor_endpoint)
    {
        return;
    }

    uint64_t result;
    std::lock_guard<std::mutex> lock(this->lock);

    if (this->start_times.find(timer_id) == this->start_times.end())
    {
        return;
    }

    result = this->clock->microseconds_since_epoch() - start_times.at(timer_id);
    this->start_times.erase(timer_id);

    auto stat_string = this->scope_prefix + "." + statistic_names.at(stat) + ":" + std::to_string(result) + "|us";
    this->send(stat_string);
}

void
monitor::send_counter(bzn::statistic stat, uint64_t amount)
{
    if (!this->monitor_endpoint)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(this->lock);
    if (this->options->get_simple_options().get<bool>(option_names::MONITOR_COLLATE))
    {
        this->accumulated_stats.insert(std::make_pair(stat, 0)); // silently fails if key exists
        this->accumulated_stats.insert_or_assign(stat, this->accumulated_stats.at(stat) + amount);
        this->maybe_send();
    }
    else
    {
        this->send(this->format_counter(stat, amount));
    }
}

std::string
monitor::format_counter(bzn::statistic stat, uint64_t amount)
{
    return this->scope_prefix + "." + statistic_names.at(stat) + ":" + std::to_string(amount) + "|c";
}

void
monitor::send(const std::string& stat)
{
//    LOG(debug) << "..." << (stat.length() <= 30 ? stat : stat.substr(stat.length() - 30));
    std::shared_ptr<boost::asio::const_buffer> buffer = std::make_shared<boost::asio::const_buffer>(stat.c_str(), stat.size());
    this->socket->async_send_to(*buffer, *(this->monitor_endpoint),
            [buffer](const boost::system::error_code& ec, std::size_t /*bytes*/)
            {
                if (ec)
                {
                    LOG(error) << "UDP send failed";
                }
            });
}

void
monitor::maybe_send()
{
    auto threshold = this->time_last_sent + this->options->get_simple_options().get<uint64_t>(option_names::MONITOR_COLLATE_INTERVAL_SECONDS)*1000000;
    if (this->clock->microseconds_since_epoch() >= threshold)
    {
//        LOG(debug) << "Sending batched statistics:";
        for (const auto& pair : this->accumulated_stats)
        {
            this->send(this->format_counter(pair.first, pair.second));
        }
        this->accumulated_stats.clear();
        this->time_last_sent = this->clock->microseconds_since_epoch();
    }
}