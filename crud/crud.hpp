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

        void start() override;

    private:
        void handle_ws_crud_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session);

        void set_leader_info(bzn::message& msg);

        void do_raft_task_routing(const bzn::message &msg, bzn::message &response);

        void do_candidate_tasks(const bzn::message& request, bzn::message& response);
        void do_follower_tasks(const bzn::message& request, bzn::message& response);
        void do_leader_tasks(const bzn::message& request, bzn::message& response);

        void handle_create(const bzn::message &msg, bzn::message &response);
        void handle_read(const bzn::message& msg, bzn::message& response);
        void handle_update(const bzn::message &request, bzn::message &response);
        void handle_delete(const bzn::message &request, bzn::message &response);

        void commit_create(const bzn::message &msg);
        void commit_update(const bzn::message &msg);
        void commit_delete(const bzn::message &msg);

        // TODO move get keys out of crud.
        void handle_get_keys(const bzn::message& msg, bzn::message& response);
        void handle_has(const bzn::message& msg, bzn::message& response);
        void handle_size(const bzn::message& msg, bzn::message& response);

        void register_route_handlers();
        void register_crud_command_handlers();
        void register_utility_command_handlers();
        void register_commit_handlers();

        bool validate_create_or_update(const bzn::message &request);
        bool validate_read_or_delete(const bzn::message &request);

        std::shared_ptr<bzn::raft_base>    raft;
        std::shared_ptr<bzn::node_base>    node;
        std::shared_ptr<bzn::storage_base> storage;

        using route_handler_t = std::function<void(const bzn::message& request, bzn::message& response)>;
        using commit_handler_t = std::function<void(const bzn::message& msg)>;
        using command_handler_t = std::function<void(const bzn::message& msg, bzn::message& resp)>;

        std::unordered_map<bzn::raft_state, route_handler_t> route_handlers;
        std::unordered_map<std::string, commit_handler_t> commit_handlers;
        std::unordered_map<std::string, command_handler_t> command_handlers;
        std::unordered_map<std::string, command_handler_t> utility_handlers;

        std::once_flag start_once;
    };

} // bzn
