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

#include <include/bluzelle.hpp>
#include <crud/crud_base.hpp>
#include <crud/subscription_manager_base.hpp>
#include <node/node_base.hpp>
#include <storage/storage_base.hpp>


namespace bzn
{
    class crud final : public bzn::crud_base, public std::enable_shared_from_this<crud>
    {
    public:
        crud(std::shared_ptr<bzn::storage_base> storage, std::shared_ptr<bzn::subscription_manager_base> subscription_manager);

        void handle_request(const database_msg& request, const std::shared_ptr<bzn::session_base>& session) override;

        void start() override;

    private:

        void handle_create(const database_msg& request, std::shared_ptr<bzn::session_base> session);

        void handle_read(const database_msg& request, std::shared_ptr<bzn::session_base> session);

        void handle_update(const database_msg& request, std::shared_ptr<bzn::session_base> session);

        void handle_delete(const database_msg& request, std::shared_ptr<bzn::session_base> session);

        void handle_has(const database_msg& request, std::shared_ptr<bzn::session_base> session);

        void handle_keys(const database_msg& request, std::shared_ptr<bzn::session_base> session);

        void handle_size(const database_msg& request, std::shared_ptr<bzn::session_base> session);

        void send_response(const database_msg& request, bzn::storage_base::result result, database_response&& response,
            std::shared_ptr<bzn::session_base>& session);

        std::shared_ptr<bzn::storage_base> storage;
        std::shared_ptr<bzn::subscription_manager_base> subscription_manager;

        using message_handler_t = std::function<void(const database_msg& request, std::shared_ptr<bzn::session_base> session)>;

        std::unordered_map<database_msg::MsgCase, message_handler_t> message_handlers;
    };

} // namespace bzn
