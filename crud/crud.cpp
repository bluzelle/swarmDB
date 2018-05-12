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
    const std::set<std::string> accepted_crud_commands{bzn::MSG_CMD_CREATE, bzn::MSG_CMD_READ, bzn::MSG_CMD_UPDATE, bzn::MSG_CMD_DELETE,
                                                 bzn::MSG_CMD_KEYS, bzn::MSG_CMD_HAS, bzn::MSG_CMD_SIZE};
}


crud::crud(std::shared_ptr<bzn::node_base> node, std::shared_ptr<bzn::raft_base> raft, std::shared_ptr<bzn::storage_base> storage)
        : raft(std::move(raft))
        , node(std::move(node))
        , storage(std::move(storage))
{
    this->register_route_handlers();
    this->register_crud_command_handlers();
    this->register_utility_command_handlers();
    this->register_commit_handlers();
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
                    if (auto search = self->commit_handlers.find(msg["cmd"].asString());search != self->commit_handlers.end())
                    {
                        search->second(msg);
                    }
                    return true;
                });
        });
}


void
crud::do_raft_task_routing(const bzn::message& request, bzn::message& response)
{
    // TODO: This module can be simplified by removing the need for this method. We can/do check state *inside* of the command handlers.
    auto search = this->route_handlers.find(raft->get_state());

    if (search==this->route_handlers.end())
    {
        response["error"] = bzn::MSG_INVALID_RAFT_STATE;
    }
    else
    {
        search->second(request, response);
    }
}


void
crud::handle_create(const bzn::message &request, bzn::message &response)
{
    if(!this->validate_create(request))
    {
        response["error"] = bzn::MSG_INVALID_ARGUMENTS;
    }
    else
    {
        if(this->storage->has(request["db-uuid"].asString(), request["data"]["key"].asString()))
        {
            response["error"] = bzn::MSG_RECORD_EXISTS;
        }
        else
        {
            this->raft->append_log(request);
        }
    }
}


void
crud::commit_create(const bzn::message &msg)
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
crud::handle_read(const bzn::message& request, bzn::message& response)
{
    if(!validate_read_xor_delete(request))
    {
        response["error"] = bzn::MSG_INVALID_ARGUMENTS;
    }
    else
    {
        auto record = this->storage->read(request["db-uuid"].asString(), request["data"]["key"].asString());

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
}


void
crud::handle_update(const bzn::message &request, bzn::message &response)
{
    if(!this->validate_update(request))
    {
        response["error"] = bzn::MSG_INVALID_ARGUMENTS;
    }
    else
    {
        if(!this->storage->has(request["db-uuid"].asString(), request["data"]["key"].asString()))
        {
            response["error"] = bzn::MSG_RECORD_NOT_FOUND;
        }
        else
        {
            this->raft->append_log(request);
        }
    }
}


void
crud::commit_update(const bzn::message &msg)
{
    storage_base::result result = this->storage->update(msg["db-uuid"].asString(), msg["data"]["key"].asString(), msg["data"]["value"].asString());

    if(storage_base::result::ok != result)
    {
        LOG(error) << "Request:"
                   << msg["request-id"].asString()
                   << " Update failed with error ["
                   << this->storage->error_msg(result)
                   << "]";
    }
}


void
crud::handle_delete(const bzn::message &request, bzn::message &response)
{
    if(!validate_read_xor_delete(request))
    {
        response["error"] = bzn::MSG_INVALID_ARGUMENTS;
    }
    else
    {
        if (this->storage->has(request["db-uuid"].asString(), request["data"]["key"].asString()))
        {
            this->raft->append_log(request);
        }
        else
        {
            response["error"] = bzn::MSG_RECORD_NOT_FOUND;
        }
    }
}


void
crud::commit_delete(const bzn::message &msg)
{
    storage_base::result result = this->storage->remove(msg["db-uuid"].asString(), msg["data"]["key"].asString());

    if(storage_base::result::ok!=result)
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
crud::handle_ws_crud_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
{
    auto response = std::make_shared<bzn::message>();
    (*response)["request-id"] = msg["request-id"];

    if( !(msg.isMember("bzn-api") && msg["bzn-api"]=="crud"))
    {
        (*response)["error"] = bzn::MSG_INVALID_CRUD_COMMAND;
    }
    else
    {
        if( auto search = accepted_crud_commands.find(msg["cmd"].asString()); search != accepted_crud_commands.end() )
        {
            this->do_raft_task_routing(msg, *response);
        }
        else
        {
            (*response)["error"] = bzn::MSG_INVALID_CRUD_COMMAND;
        }
    }
    session->send_message(response, nullptr);
}


void
crud::do_candidate_tasks(const bzn::message& /*request*/, bzn::message& response)
{
    response["error"] = bzn::MSG_ELECTION_IN_PROGRESS;
}


void
crud::do_follower_tasks(const bzn::message& request, bzn::message& response)
{
    std::string cmd = request["cmd"].asString();

    if(cmd==bzn::MSG_CMD_READ)
    {
        this->command_handlers[bzn::MSG_CMD_READ](request, response);
    }
    // TODO: Move utilities into their own module.
    else if (auto search = this->utility_handlers.find(cmd); search != this->utility_handlers.end())
    {
        search->second(request, response);
    }
    else
    {
        response["error"] = bzn::MSG_NOT_THE_LEADER;
        this->set_leader_info(response);
    }
}


void
crud::do_leader_tasks(const bzn::message& request, bzn::message& response)
{
    if(request.isMember("cmd"))
    {
        std::string command = request["cmd"].asString();
        if(auto ch = this->command_handlers.find(command); ch != this->command_handlers.end())
        {
            ch->second(request, response);
        }
        // TODO: Move utilities into their own module
        else if(auto uh = this->utility_handlers.find(command); uh != this->utility_handlers.end())
        {
            uh->second(request, response);
        }
        else
        {
            this->raft->append_log(request);
        }
    }
}


void crud::register_route_handlers()
{
    this->route_handlers[bzn::raft_state::leader] = std::bind(&crud::do_leader_tasks, this, std::placeholders::_1, std::placeholders::_2);
    this->route_handlers[bzn::raft_state::candidate] = std::bind(&crud::do_candidate_tasks, this, std::placeholders::_1, std::placeholders::_2);
    this->route_handlers[bzn::raft_state::follower] = std::bind(&crud::do_follower_tasks, this, std::placeholders::_1, std::placeholders::_2);
}


void crud::register_crud_command_handlers()
{
    // CRUD command handlers - these handlers accept incoming CRUD commands
    // and, based on the current state of the daemon, choose to ignore or act
    // Daemons in the follower state, for example will defer CUD commands to
    // the leader, but do the Read command work.
    // Leaders will seek concensus from RAFT for all CRUD commands but the do
    // the READ work.
    // Candidates will refuse all commands.
    this->command_handlers[bzn::MSG_CMD_CREATE] = std::bind(&crud::handle_create, this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_READ]   = std::bind(&crud::handle_read,   this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_UPDATE] = std::bind(&crud::handle_update, this, std::placeholders::_1, std::placeholders::_2);
    this->command_handlers[bzn::MSG_CMD_DELETE] = std::bind(&crud::handle_delete, this, std::placeholders::_1, std::placeholders::_2);
}


void crud::register_commit_handlers()
{
    // CUD commands are committed when the swarm reaches concensus via RAFT.
    // Note that READ does not require a commit as no data is being changed.
    this->commit_handlers[bzn::MSG_CMD_CREATE] = std::bind(&crud::commit_create, this, std::placeholders::_1);
    this->commit_handlers[bzn::MSG_CMD_UPDATE] = std::bind(&crud::commit_update, this, std::placeholders::_1);
    this->commit_handlers[bzn::MSG_CMD_DELETE] = std::bind(&crud::commit_delete, this, std::placeholders::_1);
}


void crud::register_utility_command_handlers()
{
    // Utility command handlers - these handlers accept incoming non RAFT
    // utility commands. They do not require concensus and can be applied
    // to any node in a leader or follower state.
    // TODO: move these commands into thier own class
    this->utility_handlers[bzn::MSG_CMD_KEYS]   = std::bind(&crud::handle_get_keys,       this, std::placeholders::_1, std::placeholders::_2);
    this->utility_handlers[bzn::MSG_CMD_HAS]    = std::bind(&crud::handle_has,            this, std::placeholders::_1, std::placeholders::_2);
    this->utility_handlers[bzn::MSG_CMD_SIZE]   = std::bind(&crud::handle_size,           this, std::placeholders::_1, std::placeholders::_2);
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


bool
crud::validate_create(const bzn::message &msg)
{
    return msg.isMember("db-uuid") && msg.isMember("data") && msg["data"].isMember("key") && msg["data"].isMember("value");
}


bool
crud::validate_read_xor_delete(const bzn::message &request)
{
    return request.isMember("db-uuid") && request.isMember("data") && request["data"].isMember("key");
}


bool
crud::validate_update(const bzn::message &msg)
{
    return msg.isMember("db-uuid") && msg.isMember("data") && msg["data"].isMember("key") && msg["data"].isMember("value");
}
