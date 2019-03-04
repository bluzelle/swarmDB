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
#include <pbft/operations/pbft_operation.hpp>
#include <queue>
#include <gtest/gtest_prod.h>

namespace bzn
{
    const uint32_t    max_completed_requests_memory {10000}; // TODO - consider making this a configurable option

    class pbft_failure_detector : public std::enable_shared_from_this<pbft_failure_detector>, public bzn::pbft_failure_detector_base
    {
    public:
        pbft_failure_detector(std::shared_ptr<bzn::asio::io_context_base>);

        void request_seen(const bzn::hash_t& req_hash) override;

        void request_executed(const bzn::hash_t& req_hash) override;

        void register_failure_handler(std::function<void()> handler) override;

        FRIEND_TEST(pbft_failure_detector_test, add_completed_request_hash_must_update_completed_requests_and_completed_request_queue);
        FRIEND_TEST(pbft_failure_detector_test, add_completed_request_hash_must_garbage_collect);

    private:

        void start_timer();
        void handle_timeout(boost::system::error_code ec);

        void add_completed_request_hash(const bzn::hash_t& request_hash);

        std::shared_ptr<bzn::asio::io_context_base> io_context;

        std::unique_ptr<bzn::asio::steady_timer_base> request_progress_timer;

        std::list<bzn::hash_t> ordered_requests;
        std::unordered_set<bzn::hash_t> outstanding_requests;
        std::unordered_set<bzn::hash_t> completed_requests;
        std::queue<bzn::hash_t> completed_request_queue;

        std::function<void()> failure_handler;

        std::mutex lock;
    };

}
