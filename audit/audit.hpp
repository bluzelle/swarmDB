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

#include <audit/audit_base.hpp>
#include <node/node_base.hpp>
#include <proto/bluzelle.pb.h>
#include <include/boost_asio_beast.hpp>
#include <mutex>
#include <boost/asio/ip/udp.hpp>
#include <include/optional.hpp>


namespace bzn
{

    class audit : public audit_base, public std::enable_shared_from_this<audit>
    {
    public:
        audit(std::shared_ptr<bzn::asio::io_context_base>
                , std::shared_ptr<bzn::node_base> node
                , bzn::optional<boost::asio::ip::udp::endpoint>);

        size_t error_count() const override;

        const std::list<std::string>& error_strings() const override;

        void handle(const bzn::message& message, std::shared_ptr<bzn::session_base> session) override;
        void handle_commit(const commit_notification&) override;
        void handle_leader_status(const leader_status&) override;

        void start() override;

    private:

        void handle_leader_alive_timeout(const boost::system::error_code& ec);
        void handle_leader_progress_timeout(const boost::system::error_code& ec);

        void reset_leader_alive_timer();
        void reset_leader_progress_timer();
        void clear_leader_progress_timer();

        void report_error(const std::string& short_name, const std::string& error_description);
        void send_to_monitor(const std::string& stat);
        
        void handle_leader_data(const leader_status&);
        void handle_leader_made_progress(const leader_status&);

        std::list<std::string> recorded_errors;
        const std::shared_ptr<bzn::node_base> node;
        const std::shared_ptr<bzn::asio::io_context_base> io_context;

        uint leader_dead_count = 0;
        uint leader_stuck_count = 0;

        std::map<uint64_t, bzn::uuid_t> recorded_leaders;
        std::map<uint64_t, std::string> recorded_commits;

        std::once_flag start_once;
        std::mutex audit_lock;
        std::unique_ptr<bzn::asio::steady_timer_base> leader_alive_timer;
        std::unique_ptr<bzn::asio::steady_timer_base> leader_progress_timer;

        // TODO: Make this configurable
        std::chrono::milliseconds leader_timeout{std::chrono::milliseconds(30000)};

        bzn::uuid_t last_leader = "";
        uint64_t last_leader_commit_index = 0;
        bool leader_has_uncommitted_entries = false;

        bzn::optional<boost::asio::ip::udp::endpoint> monitor_endpoint;
        std::unique_ptr<bzn::asio::udp_socket_base> socket;
    };

}
