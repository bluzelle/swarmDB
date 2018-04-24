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

#include <storage/storage.hpp>

using namespace bzn;

namespace
{
    // todo: replace with protobuf definition...
    const std::string CRUD_MSG_TYPE = "crud";
}


storage::storage(std::shared_ptr<bzn::node_base> node)
    : node(std::move(node))
{
}


void
storage::create(const bzn::uuid_t& /*uuid*/, const std::string& /*key*/, const std::string& /*value*/)
{

}


std::shared_ptr<bzn::storage_base::record>
storage::read(const bzn::uuid_t& /*uuid*/, const std::string& /*key*/)
{
    return nullptr;
}


void
storage::update(const bzn::uuid_t& /*uuid*/, const std::string& /*key*/, const std::string& /*value*/)
{

}


void
storage::remove(const bzn::uuid_t& /*uuid*/, const std::string& /*key*/)
{

}


void
storage::priv_msg_handler(const bzn::message& /*msg*/, std::shared_ptr<bzn::session_base> /*session*/)
{

}


void
storage::start()
{
    this->node->register_for_message(CRUD_MSG_TYPE, std::bind(&storage::priv_msg_handler,
        shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}
