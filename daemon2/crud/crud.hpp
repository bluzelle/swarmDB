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


namespace bzn
{
    class crud final : public bzn::crud_base
    {
    public:
        crud(std::shared_ptr<bzn::node_base> node, std::shared_ptr<bzn::raft_base> raft, std::shared_ptr<bzn::storage_base> storage);

    private:
        void handle_ws_crud_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session);

        std::shared_ptr<bzn::raft_base>    raft;
        std::shared_ptr<bzn::storage_base> storage;
    };

} // bzn
