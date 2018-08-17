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

#include <pbft/pbft_failure_detector_base.hpp>
#include <include/boost_asio_beast.hpp>
#include <pbft/pbft_operation.hpp>

namespace bzn
{

    class pbft_failure_detector : public std::enable_shared_from_this<pbft_failure_detector>, public bzn::pbft_failure_detector_base
    {
    public:
        pbft_failure_detector(std::shared_ptr<bzn::asio::io_context_base>);

        void request_seen(const pbft_request& req) override;

        void request_executed(const pbft_request& req) override;

        void register_failure_handler(std::function<void()> handler) override;

    private:

        void start_timer();
        void handle_timeout(boost::system::error_code ec);

        std::shared_ptr<bzn::asio::io_context_base> io_context;

        std::unique_ptr<bzn::asio::steady_timer_base> request_progress_timer;

        std::list<pbft_request> ordered_requests;
        std::unordered_set<bzn::hash_t> outstanding_requests;
        std::unordered_set<bzn::hash_t> completed_requests;

        std::function<void()> failure_handler;

        std::mutex lock;
    };

}
