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

#include <crud/crud.hpp>

using namespace bzn;


crud::crud(std::shared_ptr<bzn::node_base> node, std::shared_ptr<bzn::raft_base> raft, std::shared_ptr<bzn::storage_base> storage)
    : raft(std::move(raft))
    , storage(std::move(storage))
{
    // todo: handle failure with exception!
    node->register_for_message("crud", std::bind(&crud::handle_ws_crud_messages, this, std::placeholders::_1, std::placeholders::_2));

    this->raft->register_commit_handler(
        [](const auto& msg)
        {
            LOG(info) << " commit: " << msg.toStyledString();

            return true;
        });
}


void
crud::handle_ws_crud_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> /*session*/)
{
    LOG(info) << msg.toStyledString();

    // todo: if raft not leader and cud then return error to the session that I'm not the leader.

    this->raft->append_log(msg);
}
