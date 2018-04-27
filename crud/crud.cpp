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
    std::set<std::string> accepted_crud_commands{bzn::MSG_CMD_CREATE, bzn::MSG_CMD_READ, bzn::MSG_CMD_UPDATE, bzn::MSG_CMD_DELETE,
                                                 bzn::MSG_CMD_KEYS, bzn::MSG_CMD_HAS, bzn::MSG_CMD_SIZE};
}

crud::crud(std::shared_ptr<bzn::node_base> node, std::shared_ptr<bzn::raft_base> raft, std::shared_ptr<bzn::storage_base> storage)
        : raft(std::move(raft))
        , node(std::move(node))
        , storage(std::move(storage))
{
    this->handlers[bzn::MSG_CMD_CREATE] = std::bind(&crud::handle_create, this, std::placeholders::_1);
    this->handlers[bzn::MSG_CMD_UPDATE] = std::bind(&crud::handle_update, this, std::placeholders::_1);
    this->handlers[bzn::MSG_CMD_DELETE] = std::bind(&crud::handle_delete, this, std::placeholders::_1);

    this->command_handlers[bzn::MSG_CMD_CREATE] = std::bind(&crud::handle_create_command, this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_READ]   = std::bind(&crud::handle_read,           this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_UPDATE] = std::bind(&crud::handle_update_command, this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_DELETE] = std::bind(&crud::leader_delete_task,    this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_KEYS]   = std::bind(&crud::handle_get_keys,       this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_HAS]    = std::bind(&crud::handle_has,            this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_SIZE]   = std::bind(&crud::handle_size,           this, std::placeholders::_1, std::placeholders::_2);
}


void
crud::start()
{
    std::call_once(this->start_once,
        [this]()
        {
            // This handler deals with messages coming from the user.
            if (!this->node->register_for_message("crud",
                                                  std::bind(&crud::handle_ws_crud_messages, shared_from_this(), std::placeholders::_1,std::placeholders::_2)))
            {
                throw std::runtime_error("Unable to register for CRUD messages!");
            }

            // the commit handler deals with tasks that require concensus from RAFT
            this->raft->register_commit_handler(
                [self = shared_from_this()](const bzn::message& msg)
                {
                    LOG(debug) << " commit: " << msg.toStyledString();

                    if (auto search = self->handlers.find(msg["cmd"].asString());search != self->handlers.end())
                    {
                        search->second(msg);
                    }
                    return true;
                });


        });
}

void
crud::set_leader_info(bzn::message& msg)
{
    peer_address_t leader = this->raft->get_leader();
    msg["data"]["leader-id"] = leader.uuid;
    msg["data"]["leader-host"] = leader.host;
    msg["data"]["leader-port"] = leader.port;
    msg["data"]["leader-name"] = leader.name;
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
crud::handle_create_command(const bzn::message& msg, bzn::message& response)
{
    if(msg.isMember("db-uuid") && msg.isMember("data") && msg["data"].isMember("key") && msg["data"].isMember("value"))
    {
        if(this->storage->has(msg["db-uuid"].asString(), msg["data"]["key"].asString()))
        {
            response["error"] = bzn::MSG_RECORD_EXISTS;
        }
        else
        {
            this->raft->append_log(msg);
        }
    }
    else
    {
        response["error"] = bzn::MSG_INVALID_ARGUMENTS;
    }
}


void
crud::handle_update_command(const bzn::message& msg, bzn::message& response)
{
    if(msg.isMember("db-uuid") && msg.isMember("data") && msg["data"].isMember("key") && msg["data"].isMember("value"))
    {
        if(!this->storage->has(msg["db-uuid"].asString(), msg["data"]["key"].asString()))
        {
            response["error"] = bzn::MSG_RECORD_NOT_FOUND;
        }
        else
        {
            this->raft->append_log(msg);
        }
    }
    else
    {
        response["error"] = bzn::MSG_INVALID_ARGUMENTS;
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
            this->set_leader_info(response);
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

    response["data"][bzn::MSG_CMD_KEYS] = json_keys;
}

void
crud::handle_has(const bzn::message& msg, bzn::message& response)
{
    if(msg.isMember("db-uuid") && msg.isMember("data") && msg["data"].isMember("key"))
    {
        response["data"]["key-exists"] = this->storage->has(msg["db-uuid"].asString(), msg["data"]["key"].asString());
    }
    else
    {
        response["error"] = bzn::MSG_INVALID_ARGUMENTS;
    }
}

void
crud::handle_size(const bzn::message& msg, bzn::message& response)
{
    response["data"]["size"] = Json::UInt64(this->storage->get_size(msg["db-uuid"].asString()));
}

void
crud::do_raft_task_routing(const bzn::message& msg, bzn::message& response)
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
        default: // A bad RAFT State?!?!?! This should not happen
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
        this->do_raft_task_routing(msg, response);
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

    std::string cmd = msg["cmd"].asString();

    // TODO: replace this if else tree with the strategy pattern.
    if(cmd==bzn::MSG_CMD_READ)
    {
        this->handle_read(msg, response);
    }
    else if(cmd==bzn::MSG_CMD_KEYS)
    {
        this->handle_get_keys(msg, response);
    }
    else if(cmd==bzn::MSG_CMD_HAS)
    {
        this->handle_has(msg, response);
    }
    else if(cmd==bzn::MSG_CMD_SIZE)
    {
        this->handle_size(msg, response);
    }
    else
    {
        response["error"] = bzn::MSG_NOT_THE_LEADER;
        this->set_leader_info(response);
    }
    return response;
}

void
crud::leader_delete_task(const bzn::message &msg, bzn::message &response)
{
    if (this->storage->has(msg["db-uuid"].asString(), msg["data"]["key"].asString()))
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

    if(msg.isMember("cmd"))
    {
        const auto cmd = msg["cmd"].asString();

        const auto command_handler = this->command_handlers.find(cmd);

        if (command_handler == this->command_handlers.end())
        {
            this->raft->append_log(msg);
        }
        else
        {
            command_handler->second(msg, response);
        }
    }
    else if(msg["cmd"].asString()==bzn::MSG_CMD_SIZE)
    {
        this->handle_size(msg, response);
    }
    else
    {
        response["error"] = bzn::MSG_COMMAND_NOT_SET;
    }

    return response;
}
