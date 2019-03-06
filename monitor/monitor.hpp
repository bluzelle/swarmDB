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

#include <monitor/monitor_base.hpp>
#include <options/options_base.hpp>
#include <include/boost_asio_beast.hpp>
#include <include/system_clock.hpp>

#include <list>
#include <unordered_map>

namespace bzn
{
    class monitor : public monitor_base
    {
    public:

        monitor(std::shared_ptr<bzn::options_base> options, std::shared_ptr<bzn::asio::io_context_base> context, std::shared_ptr<bzn::system_clock_base> clock);

        void start_timer(std::string instance_id) override;

        void finish_timer(statistic stat, std::string instance_id) override;

        void send_counter(statistic stat, uint64_t amount = 1) override;

    private:

        void send(const std::string& stat);

        std::list<std::string> ordered_timers;
        std::unordered_map<std::string, uint64_t> start_times;
        std::mutex timers_lock;

        std::shared_ptr<bzn::options_base> options;
        std::shared_ptr<bzn::asio::io_context_base> context;
        std::shared_ptr<bzn::system_clock_base> clock;
        std::unique_ptr<bzn::asio::udp_socket_base> socket;
        const std::optional<boost::asio::ip::udp::endpoint> monitor_endpoint;
        const std::string scope_prefix;
    };
}
