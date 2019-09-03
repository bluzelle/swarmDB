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
#include <utils/bytes_to_debug_string.hpp>

using namespace bzn;

pbft_failure_detector::pbft_failure_detector(std::shared_ptr<bzn::asio::io_context_base> io_context
    , std::shared_ptr<bzn::options_base> options)
        : io_context(std::move(io_context))
        , options(options)
        , request_progress_timer(this->io_context->make_unique_steady_timer())
{
}

void
pbft_failure_detector::start_timer()
{
    this->request_progress_timer->expires_from_now(this->options->get_fd_oper_timeout());
    this->request_progress_timer->async_wait(std::bind(&pbft_failure_detector::handle_timeout, shared_from_this(), std::placeholders::_1));
}

void
pbft_failure_detector::handle_timeout(boost::system::error_code ec)
{
    if (ec)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(this->lock);

    assert(!this->ordered_requests.empty());

    if (this->completed_requests.count(this->ordered_requests.front())  == 0)
    {
        LOG(error) << "Failure detector detected unexecuted request "
            << bzn::bytes_to_debug_string(this->ordered_requests.front()) << '\n';
        this->ordered_requests.pop_front();
        if (this->ordered_requests.size() > 0)
        {
            LOG(debug) << "handle_failure starting secondary failure timer";
            this->request_progress_timer->expires_from_now(this->options->get_fd_fail_timeout());
            this->request_progress_timer->async_wait(std::bind(&pbft_failure_detector::handle_timeout, shared_from_this(), std::placeholders::_1));
        }

        this->io_context->post(std::bind(this->failure_handler));
        return;
    }

    while (this->ordered_requests.size() > 0 &&
           this->completed_requests.count(this->ordered_requests.front()) > 0)
    {
        this->ordered_requests.pop_front();
    }

    if (this->ordered_requests.size() > 0)
    {
        LOG(debug) << "handle_timeout starting timer";
        this->start_timer();
    }
}

void
pbft_failure_detector::request_seen(const bzn::hash_t& req_hash)
{
    std::lock_guard<std::mutex> lock(this->lock);

    if (this->outstanding_requests.count(req_hash) == 0 && this->completed_requests.count(req_hash) == 0)
    {
        LOG(debug) << "Failure detector recording new request " << bzn::bytes_to_debug_string(req_hash) << '\n';
        this->ordered_requests.emplace_back(req_hash);
        this->outstanding_requests.emplace(req_hash);

        if (this->ordered_requests.size() == 1)
        {
            LOG(debug) << "request_seen starting timer";
            this->start_timer();
        }
    }
}

void
pbft_failure_detector::request_executed(const bzn::hash_t& req_hash)
{
    std::lock_guard<std::mutex> lock(this->lock);
    if (this->outstanding_requests.count(req_hash))
    {
        LOG(debug) << "Failure detector executed request " << bzn::bytes_to_debug_string(req_hash) << '\n';
    }

    this->outstanding_requests.erase(req_hash);
    this->add_completed_request_hash(req_hash);
}

void
pbft_failure_detector::register_failure_handler(std::function<void()> handler)
{
    std::lock_guard<std::mutex> lock(this->lock);

    this->failure_handler = handler;
}

void
pbft_failure_detector::add_completed_request_hash(const bzn::hash_t& request_hash)
{
    this->completed_requests.emplace(request_hash);
    this->completed_request_queue.push(request_hash);

    if (max_completed_requests_memory < this->completed_requests.size())
    {
        auto old_hash = this->completed_request_queue.front();
        this->completed_request_queue.pop();
        this->completed_requests.erase(old_hash);
    }
}