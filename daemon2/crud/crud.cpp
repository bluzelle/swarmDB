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
#include <numeric>

using namespace bzn;

namespace
{
    std::set<std::string> accepted_crud_commands{"create", "read", "update", "delete", "get_keys"};
}

crud::crud(std::shared_ptr<bzn::node_base> node, std::shared_ptr<bzn::raft_base> raft, std::shared_ptr<bzn::storage_base> storage)
        : raft(std::move(raft))
        , storage(std::move(storage))
{
    if(!node->register_for_message("crud", std::bind(&crud::handle_ws_crud_messages, this, std::placeholders::_1, std::placeholders::_2)))
    {
        throw std::runtime_error("Unable to register for CRUD messages!");
    }

    this->handlers["create"] = std::bind(&crud::handle_create, this, std::placeholders::_1);
    this->handlers["update"] = std::bind(&crud::handle_update, this, std::placeholders::_1);
    this->handlers["delete"] = std::bind(&crud::handle_delete, this, std::placeholders::_1);

    this->raft->register_commit_handler(
        [&](const bzn::message& msg)
        {
            LOG(debug) << " commit: " << msg.toStyledString();

            if(auto search = this->handlers.find(msg["cmd"].asString());search != this->handlers.end())
            {
                search->second(msg);
            }
            return true;
        });
}

void
crud::handle_create(const bzn::message& msg)
{
    storage_base::result result = this->storage->create(msg["db-uuid"].asString(), msg["data"]["key"].asString(), msg["data"]["value"].asString());

    if(storage_base::result::ok == result)
    {
        LOG(info) << "Request:" << msg["request-id"].asString() << " Create successful.";
    }
    else
    {
        LOG(error) << "Request:"
                   << msg["request-id"].asString()
                   << " Create failed with error ["
                   << this->storage->error_msg(result)
                   << "]";
    }
}

void
crud::handle_read(const bzn::message& msg, bzn::message& response)
{
    // followers can only read.
    auto record = this->storage->read(msg["db-uuid"].asString(), msg["data"]["key"].asString());

    if (record)
    {
        response["data"]["value"] = record->value;
        response["error"] = bzn::MSG_OK;
    }
    else
    {
        if(this->raft->get_state()==bzn::raft_state::leader)
        {
            response["error"] = bzn::MSG_RECORD_NOT_FOUND;
        }
        else
        {
            response["error"] = bzn::MSG_VALUE_DOES_NOT_EXIST;
            response["data"]["leader_id"] = this->raft->get_leader();
        }
    }
}

void
crud::handle_update(const bzn::message& msg)
{
    LOG(debug) << msg.toStyledString();

    storage_base::result result = this->storage->update(msg["db-uuid"].asString(), msg["data"]["key"].asString(), msg["data"]["value"].asString());

    if(storage_base::result::ok == result)
    {
        LOG(debug) << "Request:" << msg["request-id"].asString() << " Update successful.";
    }
    else
    {
        LOG(error) << "Request:"
                   << msg["request-id"].asString()
                   << " Update failed with error ["
                   << this->storage->error_msg(result)
                   << "]";
    }
}

void
crud::handle_delete(const bzn::message& msg)
{
    LOG(debug) << msg.toStyledString();

    storage_base::result result = this->storage->remove(msg["db-uuid"].asString(), msg["data"]["key"].asString());

    if(storage_base::result::ok==result)
    {
        LOG(debug) << "Request:" << msg["request-id"].asString() << " Delete successful.";
    }
    else
    {
        LOG(error) << "Request:"
                   << msg["request-id"].asString()
                   << " Delete failed with error ["
                   << this->storage->error_msg(result)
                   << "]";
    }
}

void
crud::handle_get_keys(const bzn::message& msg, bzn::message& response)
{
    std::vector<std::string> keys = this->storage->get_keys(msg["db-uuid"].asString());

    Json::Value json_keys;

    for(auto& key : keys)
    {
        json_keys.append(key);
    }

    response["data"]["keys"] = json_keys;
}

void
crud::do_tasks(const bzn::message& msg, bzn::message& response)
{
    switch(raft->get_state())
    {
        case bzn::raft_state::candidate:
            response = do_candidate_tasks(msg);
            break;
        case bzn::raft_state::follower:
            response = do_follower_tasks(msg);
            break;
        case bzn::raft_state::leader:
            response = do_leader_tasks(msg);
            break;
        default: // holy crap! This should not happen
            response["request-id"] = msg["request-id"];
            response["error"] = bzn::MSG_INVALID_RAFT_STATE;
            assert(false);
            break;
    }
}

void
crud::handle_ws_crud_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
{
    LOG(debug) << msg.toStyledString();

    bzn::message response;

    assert(msg["bzn-api"]=="crud");

    if( auto search = accepted_crud_commands.find(msg["cmd"].asString()); search != accepted_crud_commands.end() )
    {
        this->do_tasks(msg,response);
    }
    else
    {
        response["request-id"] = msg["request-id"];
        response["error"] = bzn::MSG_INVALID_CRUD_COMMAND;
    }
    session->send_message(response, nullptr);
}

bzn::message
crud::do_candidate_tasks(const bzn::message& msg)
{
    bzn::message response;
    response["request-id"] = msg["request-id"];
    response["error"] = bzn::MSG_ELECTION_IN_PROGRESS;
    return response;
}

bzn::message
crud::do_follower_tasks(const bzn::message& msg)
{
    bzn::message response;
    response["request-id"] = msg["request-id"];
    response["error"] = bzn::MSG_OK;

    std::string cmd = msg["cmd"].asString();

    // TODO: replace this if else tree with the strategy pattern.
    if(cmd=="read")
    {
        this->handle_read(msg, response);
    }
    else if(cmd=="get_keys")
    {
        this->handle_get_keys(msg, response);
    }
    else
    {
        response["error"] = bzn::MSG_NOT_THE_LEADER;
        response["data"]["leader_id"] = this->raft->get_leader();
    }
    return response;
}

void
crud::leader_read_task(const bzn::message& msg, bzn::message& response)
{
    if (this->storage->read(msg["db-uuid"].asString(), msg["data"]["key"].asString()))
    {
        this->raft->append_log(msg);
    }
    else
    {
        response["error"] = bzn::MSG_RECORD_NOT_FOUND;
    }
}

bzn::message
crud::do_leader_tasks(const bzn::message& msg)
{
    LOG(debug) << msg.toStyledString();

    bzn::message response;
    response["request-id"] = msg["request-id"];
    response["error"] = bzn::MSG_OK;

    // todo: Rich - map with lamda (mini-strategy)

    if(msg["cmd"].asString()=="read")
    {
        this->handle_read(msg, response);
    }
    else if(msg["cmd"].asString()=="delete")
    {
        this->leader_read_task(msg,response);
    }
    else if(msg["cmd"].asString()=="get_keys")
    {
        this->handle_get_keys(msg, response);
    }
    else
    {
        this->raft->append_log(msg);
    }

    return response;
}
