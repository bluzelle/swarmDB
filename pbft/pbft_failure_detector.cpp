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

#include <pbft/pbft_failure_detector.hpp>

namespace
{
    const std::chrono::milliseconds operation_timeout{std::chrono::milliseconds(10000)};
}

using namespace bzn;

pbft_failure_detector::pbft_failure_detector(std::shared_ptr<bzn::asio::io_context_base> io_context)
        : io_context(std::move(io_context))
        , request_progress_timer(this->io_context->make_unique_steady_timer())
{
}

void
pbft_failure_detector::start_timer()
{
    this->request_progress_timer->expires_from_now(operation_timeout);
    this->request_progress_timer->async_wait(std::bind(&pbft_failure_detector::handle_timeout, shared_from_this(), std::placeholders::_1));
}

void
pbft_failure_detector::handle_timeout(boost::system::error_code /*ec*/)
{
    std::lock_guard<std::mutex> lock(this->lock);

    if (this->completed_requests.count(pbft_operation::request_hash(this->ordered_requests.front())) == 0)
    {
        LOG(error) << "Failure detector detected unexecuted request " << this->ordered_requests.front().ShortDebugString() << '\n';
        this->start_timer();
        this->failure_handler();
        return;
    }

    while (this->ordered_requests.size() > 0 &&
           this->completed_requests.count(pbft_operation::request_hash(this->ordered_requests.front())) > 0)
    {
        this->ordered_requests.pop_front();
    }

    if(this->ordered_requests.size() > 0)
    {
        this->start_timer();
    }
}

void
pbft_failure_detector::request_seen(const pbft_request& req)
{
    std::lock_guard<std::mutex> lock(this->lock);

    hash_t req_hash = pbft_operation::request_hash(req);

    if (this->outstanding_requests.count(req_hash) == 0 && this->completed_requests.count(req_hash) == 0)
    {
        LOG(debug) << "Failure detector recording new request " << req.ShortDebugString() << '\n';
        this->ordered_requests.emplace_back(req);
        this->outstanding_requests.emplace(req_hash);

        if(this->ordered_requests.size() == 1)
        {
            this->start_timer();
        }
    }
}

void
pbft_failure_detector::request_executed(const pbft_request& req)
{
    std::lock_guard<std::mutex> lock(this->lock);

    hash_t req_hash = pbft_operation::request_hash(req);

    this->outstanding_requests.erase(req_hash);
    this->completed_requests.emplace(req_hash);
    // TODO KEP-538: Need to garbage collect completed_requests eventually
}

void
pbft_failure_detector::register_failure_handler(std::function<void()> handler)
{
    std::lock_guard<std::mutex> lock(this->lock);

    this->failure_handler = handler;
}