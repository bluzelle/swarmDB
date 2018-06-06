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
#include <raft/raft_base.hpp>
#include <node/node_base.hpp>
#include <storage/storage_base.hpp>
#include <unordered_map>


namespace bzn
{
    class crud final : public bzn::crud_base, public std::enable_shared_from_this<crud>
    {
    public:
        crud(std::shared_ptr<bzn::node_base> node, std::shared_ptr<bzn::raft_base> raft, std::shared_ptr<bzn::storage_base> storage);

        void handle_create(const bzn::message& msg, const database_msg& request, database_response& response) override;

        void handle_read(const bzn::message& msg, const database_msg& request, database_response& response) override;

        void handle_update(const bzn::message& msg, const database_msg& request, database_response& response) override;

        void handle_delete(const bzn::message& msg, const database_msg& request, database_response& response) override;

        void start() override;

    private:
        void handle_ws_crud_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session);

        void set_leader_info(database_response& msg);

        void do_raft_task_routing(const bzn::message& msg, const database_msg& request, database_response& response);

        void do_candidate_tasks(const bzn::message& msg, const database_msg& request, database_response& response);
        void  do_follower_tasks(const bzn::message& msg, const database_msg& request, database_response& response);
        void    do_leader_tasks(const bzn::message& msg, const database_msg& request, database_response& response);

        void handle_get_keys(const bzn::message& msg, const database_msg& request, database_response& response);
        void      handle_has(const bzn::message& msg, const database_msg& request, database_response& response);
        void     handle_size(const bzn::message& msg, const database_msg& request, database_response& response);

        void commit_create(const database_msg& msg);
        void commit_update(const database_msg& msg);
        void commit_delete(const database_msg& msg);

        void register_route_handlers();
        void register_command_handlers();
        void register_commit_handlers();

        inline bool validate_value_size(size_t size)
        {
            return bzn::MAX_VALUE_SIZE < size;
        }

        std::shared_ptr<bzn::raft_base>    raft;
        std::shared_ptr<bzn::node_base>    node;
        std::shared_ptr<bzn::storage_base> storage;

        using route_handler_t   = std::function<void(const bzn::message& msg, const database_msg& request, database_response& response)>;
        using command_handler_t = std::function<void(const bzn::message& msg, const database_msg& request, database_response& response)>;
        using commit_handler_t  = std::function<void(const database_msg& msg)>;

        std::unordered_map<bzn::raft_state, route_handler_t>         route_handlers;
        std::unordered_map<database_msg::MsgCase, commit_handler_t>  commit_handlers;
        std::unordered_map<database_msg::MsgCase, command_handler_t> command_handlers;

        std::once_flag start_once;
    };

} // bzn
