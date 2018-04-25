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

        void do_raft_task_routing(const bzn::message &msg, bzn::message &response);
        bzn::message do_candidate_tasks(const bzn::message& msg);
        bzn::message do_follower_tasks(const bzn::message& msg);
        bzn::message do_leader_tasks(const bzn::message& msg);

        void leader_delete_task(const bzn::message &msg, bzn::message &response);

        void handle_create(const bzn::message& msg);
        void handle_read(const bzn::message& msg, bzn::message& response);
        void handle_update(const bzn::message& msg);
        void handle_delete(const bzn::message& msg);

        // TODO move get keys out of crud.
        void handle_get_keys(const bzn::message& msg, bzn::message& response);
        void handle_has(const bzn::message& msg, bzn::message& response);
        void handle_size(const bzn::message& msg, bzn::message& response);

        std::shared_ptr<bzn::raft_base>    raft;
        std::shared_ptr<bzn::node_base>    node;
        std::shared_ptr<bzn::storage_base> storage;

        using crud_handler_t = std::function<void(const bzn::message& msg)>;

        using commit_handler_t = std::function<void(const bzn::message& msg, bzn::message& resp)>;

        std::unordered_map<std::string, crud_handler_t> handlers;

        std::unordered_map<std::string, commit_handler_t> command_handlers;

        std::once_flag start_once;

    };

} // bzn
