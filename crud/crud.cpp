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
#include <storage/storage.hpp>

#include <boost/beast/core/detail/base64.hpp>

using namespace bzn;


crud::crud(std::shared_ptr<bzn::node_base> node, std::shared_ptr<bzn::raft_base> raft, std::shared_ptr<bzn::storage_base> storage)
        : raft(std::move(raft))
        , node(std::move(node))
        , storage(std::move(storage))
{
    this->register_route_handlers();
    this->register_command_handlers();
    this->register_commit_handlers();
}


void
crud::start()
{
    std::call_once(this->start_once,
        [this]()
        {
            // This handler deals with messages coming from the user.
            if (!this->node->register_for_message("database"
                                                  , std::bind(&crud::handle_ws_crud_messages
                                                  , shared_from_this()
                                                  , std::placeholders::_1
                                                  , std::placeholders::_2)))
            {
                throw std::runtime_error("Unable to register for DATABASE messages!");
            }

            // the commit handler deals with tasks that require concensus from RAFT
            this->raft->register_commit_handler(
                [self = shared_from_this()](const bzn::message& ws_msg)
                {
                    bzn_msg msg;

                    if (msg.ParseFromString(boost::beast::detail::base64_decode(ws_msg["msg"].asString())))
                    {
                        if (msg.msg_case() == bzn_msg::kDb)
                        {
                            if (auto search = self->commit_handlers.find(msg.db().msg_case()); search != self->commit_handlers.end())
                            {
                                search->second(msg.db());
                            }
                        }
                    }
                    else
                    {
                        LOG(error) << "failed to decode commit message:\n" << ws_msg.toStyledString().substr(0,60);
                    }

                    return true;
                });
        });
}


void
crud::do_raft_task_routing(const bzn::message& msg, const database_msg& request, database_response& response)
{
    if (auto it = this->route_handlers.find(raft->get_state()); it != this->route_handlers.end())
    {
        it->second(msg, request, response);
        return;
    }

    response.mutable_resp()->set_error(bzn::MSG_INVALID_RAFT_STATE);
}


void
crud::handle_create(const bzn::message& msg, const database_msg& request, database_response& response)
{
    if (this->validate_value_size(request.create().value().size()))
    {
        response.mutable_resp()->set_error(bzn::MSG_VALUE_SIZE_TOO_LARGE);
        return;
    }

    if (this->storage->has(request.header().db_uuid(), request.create().key()))
    {
        response.mutable_resp()->set_error(bzn::MSG_RECORD_EXISTS);
        return;
    }

    if (this->raft->get_state() == bzn::raft_state::leader)
    {
        this->raft->append_log(msg, bzn::log_entry_type::log_entry);
        return;
    }
}


void
crud::handle_read(const bzn::message& /*msg*/, const database_msg& request, database_response& response)
{
    if (auto record = this->storage->read(request.header().db_uuid(), request.read().key()); record)
    {
        response.mutable_resp()->set_value(record->value);
        return;
    }

    if (this->raft->get_state() == bzn::raft_state::leader)
    {
        response.mutable_resp()->set_error(bzn::MSG_RECORD_NOT_FOUND);
        return;
    }

    this->set_leader_info(response);
}


void
crud::handle_update(const bzn::message& msg, const database_msg& request, database_response& response)
{
    if (this->validate_value_size(request.update().value().size()))
    {
        response.mutable_resp()->set_error(bzn::MSG_VALUE_SIZE_TOO_LARGE);
        return;
    }

    if (!this->storage->has(request.header().db_uuid(), request.update().key()))
    {
        response.mutable_resp()->set_error(bzn::MSG_RECORD_NOT_FOUND);
        return;
    }

    if (this->raft->get_state() == bzn::raft_state::leader)
    {
        this->raft->append_log(msg, bzn::log_entry_type::log_entry);
        return;
    }

    this->set_leader_info(response);
}


void
crud::handle_delete(const bzn::message& msg, const database_msg& request, database_response& response)
{
    if (this->raft->get_state() != bzn::raft_state::leader)
    {
        this->set_leader_info(response);
        return;
    }

    if (this->storage->has(request.header().db_uuid(), request.delete_().key()))
    {
        this->raft->append_log(msg, bzn::log_entry_type::log_entry);
        return;
    }

    response.mutable_resp()->set_error(bzn::MSG_RECORD_NOT_FOUND);
}


void
crud::handle_get_keys(const bzn::message& /*msg*/, const database_msg& request, database_response& response)
{
    auto keys = this->storage->get_keys(request.header().db_uuid());

    if (keys.empty())
    {
        return;
    }

    for (const auto& key : keys)
    {
        response.mutable_resp()->add_keys(key);
    }
}


void
crud::handle_has(const bzn::message& /*msg*/, const database_msg& request, database_response& response)
{
    response.mutable_resp()->set_has(this->storage->has(request.header().db_uuid(), request.has().key()));
}


void
crud::handle_size(const bzn::message& /*msg*/, const database_msg& request, database_response& response)
{
    response.mutable_resp()->set_size(this->storage->get_size(request.header().db_uuid()));
}


void
crud::commit_create(const database_msg& msg)
{
    if (this->storage->create(msg.header().db_uuid(), msg.create().key(), msg.create().value()) != storage_base::result::ok)
    {
        LOG(error) << "Request:" <<msg.header().transaction_id() << " Create failed";
    }
}


void
crud::commit_update(const database_msg& msg)
{
    if (this->storage->update(msg.header().db_uuid(), msg.update().key(), msg.update().value()) != storage_base::result::ok)
    {
        LOG(error) << "Request:" << msg.header().transaction_id() << " Update failed";
    }
}


void
crud::commit_delete(const database_msg& msg)
{
    if (this->storage->remove(msg.header().db_uuid(), msg.delete_().key()) != storage_base::result::ok)
    {
        LOG(error) << "Request:" << msg.header().transaction_id() << " Delete failed";
    }
}


void
crud::handle_ws_crud_messages(const bzn::message& ws_msg, std::shared_ptr<bzn::session_base> session)
{
    bzn_msg msg;
    database_response response;

    if (!ws_msg.isMember("msg"))
    {
        LOG(error) << "Invalid message: " << ws_msg.toStyledString().substr(0,60) << "...";
        response.mutable_resp()->set_error(bzn::MSG_INVALID_CRUD_COMMAND);
        session->send_message(std::make_shared<std::string>(response.SerializeAsString()), true);
        return;
    }

    if (!msg.ParseFromString(boost::beast::detail::base64_decode(ws_msg["msg"].asString())))
    {
        LOG(error) << "Failed to decode message: " << ws_msg.toStyledString().substr(0,60) << "...";
        response.mutable_resp()->set_error(bzn::MSG_INVALID_CRUD_COMMAND);
        session->send_message(std::make_shared<std::string>(response.SerializeAsString()), true);
        return;
    }

    if (msg.msg_case() != msg.kDb)
    {
        LOG(error) << "Invalid message type: " << msg.msg_case();
        response.mutable_resp()->set_error(bzn::MSG_INVALID_ARGUMENTS);
        session->send_message(std::make_shared<std::string>(response.SerializeAsString()), true);
        return;
    }

    *response.mutable_header() = msg.db().header();

    this->do_raft_task_routing(ws_msg, msg.db(), response);

    session->send_message(std::make_shared<std::string>(response.SerializeAsString()), false);
}


void
crud::do_candidate_tasks(const bzn::message& /*msg*/, const database_msg& /*request*/, database_response& response)
{
    response.mutable_resp()->set_error(bzn::MSG_ELECTION_IN_PROGRESS);
}


void
crud::do_follower_tasks(const bzn::message& msg, const database_msg& request, database_response& response)
{
    switch(request.msg_case())
    {
        case database_msg::kRead:
        case database_msg::kKeys:
        case database_msg::kHas:
        case database_msg::kSize:
        {
            this->command_handlers[request.msg_case()](msg, request, response);
            break;
        }

        default:
        {
            this->set_leader_info(response);
            break;
        }
    }
}


void
crud::do_leader_tasks(const bzn::message& msg, const database_msg& request, database_response& response)
{
    if (auto it = this->command_handlers.find(request.msg_case()); it != this->command_handlers.end())
    {
        it->second(msg, request, response);
        return;
    }

    LOG(error) << "dropping unknown request: " << request.msg_case();
}


void crud::register_route_handlers()
{
    this->route_handlers[bzn::raft_state::leader]    = std::bind(&crud::do_leader_tasks,    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->route_handlers[bzn::raft_state::candidate] = std::bind(&crud::do_candidate_tasks, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->route_handlers[bzn::raft_state::follower]  = std::bind(&crud::do_follower_tasks,  this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}


void crud::register_command_handlers()
{
    // CRUD command handlers - these handlers accept incoming CRUD commands
    // and, based on the current state of the daemon, choose to ignore or act
    // Daemons in the follower state, for example will defer CUD commands to
    // the leader, but do the Read command work.
    // Leaders will seek concensus from RAFT for all CRUD commands but the do
    // the READ work.
    // Candidates will refuse all commands.

    this->command_handlers[database_msg::kCreate] = std::bind(&crud::handle_create,   this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->command_handlers[database_msg::kRead]   = std::bind(&crud::handle_read,     this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->command_handlers[database_msg::kUpdate] = std::bind(&crud::handle_update,   this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->command_handlers[database_msg::kDelete] = std::bind(&crud::handle_delete,   this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->command_handlers[database_msg::kKeys]   = std::bind(&crud::handle_get_keys, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->command_handlers[database_msg::kHas]    = std::bind(&crud::handle_has,      this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    this->command_handlers[database_msg::kSize]   = std::bind(&crud::handle_size,     this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}


void crud::register_commit_handlers()
{
    // CUD commands are committed when the swarm reaches concensus via RAFT.
    // Note that READ does not require a commit as no data is being changed.
    this->commit_handlers[database_msg::kCreate] = std::bind(&crud::commit_create, this, std::placeholders::_1);
    this->commit_handlers[database_msg::kUpdate] = std::bind(&crud::commit_update, this, std::placeholders::_1);
    this->commit_handlers[database_msg::kDelete] = std::bind(&crud::commit_delete, this, std::placeholders::_1);
}


void
crud::set_leader_info(database_response& msg)
{
    const auto leader = this->raft->get_leader();

    msg.mutable_redirect()->set_leader_id(leader.uuid);
    msg.mutable_redirect()->set_leader_host(leader.host);
    msg.mutable_redirect()->set_leader_port(leader.port);
    msg.mutable_redirect()->set_leader_http_port(leader.http_port);
    msg.mutable_redirect()->set_leader_name(leader.name);
}
