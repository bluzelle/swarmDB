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

#include <crud/subscription_manager_base.hpp>
#include <include/boost_asio_beast.hpp>
#include <unordered_map>
#include <mutex>
#include <list>


namespace bzn
{
    class subscription_manager final : public bzn::subscription_manager_base, public std::enable_shared_from_this<subscription_manager>
    {
    public:
        subscription_manager(std::shared_ptr<bzn::asio::io_context_base> io_context);

        void start() override;

        void   subscribe(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session) override;

        void unsubscribe(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session) override;

        void inspect_commit(const database_msg& msg) override;

    private:

        void purge_closed_sessions(const boost::system::error_code& ec);

        void notify_sessions(const bzn::uuid_t& uuid, const bzn::key_t& key, const std::string& value);

        std::unordered_map<bzn::uuid_t, std::unordered_map<bzn::key_t, std::unordered_map<bzn::session_id, std::unordered_map<uint64_t, std::weak_ptr<bzn::session_base>>>>> subscribers;

        std::mutex subscribers_lock;

        std::unique_ptr<bzn::asio::steady_timer_base> purge_timer;

        std::once_flag start_once;
    };

} // namespace bzn
