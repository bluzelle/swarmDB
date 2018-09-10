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

#include <mutex>
#include <options/options_base.hpp>
#include <include/boost_asio_beast.hpp>
#include <chaos/chaos_base.hpp>
#include <random>

namespace bzn
{
class chaos : public chaos_base, public std::enable_shared_from_this<chaos>
    {
    public:
        chaos(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::options_base> options);

        void start() override;

        bool is_message_dropped() override;
        bool is_message_delayed() override;
        void reschedule_message(std::function<void()> callback) const override;


    private:
        void start_crash_timer();
        void handle_crash_timer(const boost::system::error_code&);

        bool enabled();

        std::once_flag start_once;

        const std::shared_ptr<bzn::asio::io_context_base> io_context;
        const std::shared_ptr<bzn::options_base> options;

        std::unique_ptr<bzn::asio::steady_timer_base> crash_timer;

        std::mt19937 random;

        std::uniform_real_distribution<> random_float{0.0, 1.0};
    };
}
