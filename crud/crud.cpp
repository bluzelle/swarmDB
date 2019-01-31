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
#include <boost/algorithm/string/trim_all.hpp>
#include <utils/make_endpoint.hpp>

using namespace bzn;
using namespace std::placeholders;


namespace
{
    const std::string PERMISSION_UUID{"PERMS"};
    const std::string OWNER_KEY{"OWNER"};
    const std::string WRITERS_KEY{"WRITERS"};
}


crud::crud(std::shared_ptr<bzn::storage_base> storage, std::shared_ptr<bzn::subscription_manager_base> subscription_manager, std::shared_ptr<bzn::node_base> node)
           : storage(std::move(storage))
           , subscription_manager(std::move(subscription_manager))
           , node(std::move(node))
           , message_handlers{
                 {database_msg::kCreate,        std::bind(&crud::handle_create,         this, _1, _2, _3)},
                 {database_msg::kRead,          std::bind(&crud::handle_read,           this, _1, _2, _3)},
                 {database_msg::kUpdate,        std::bind(&crud::handle_update,         this, _1, _2, _3)},
                 {database_msg::kDelete,        std::bind(&crud::handle_delete,         this, _1, _2, _3)},
                 {database_msg::kHas,           std::bind(&crud::handle_has,            this, _1, _2, _3)},
                 {database_msg::kKeys,          std::bind(&crud::handle_keys,           this, _1, _2, _3)},
                 {database_msg::kSize,          std::bind(&crud::handle_size,           this, _1, _2, _3)},
                 {database_msg::kSubscribe,     std::bind(&crud::handle_subscribe,      this, _1, _2, _3)},
                 {database_msg::kUnsubscribe,   std::bind(&crud::handle_unsubscribe,    this, _1, _2, _3)},
                 {database_msg::kCreateDb,      std::bind(&crud::handle_create_db,      this, _1, _2, _3)},
                 {database_msg::kDeleteDb,      std::bind(&crud::handle_delete_db,      this, _1, _2, _3)},
                 {database_msg::kHasDb,         std::bind(&crud::handle_has_db,         this, _1, _2, _3)},
                 {database_msg::kWriters,       std::bind(&crud::handle_writers,        this, _1, _2, _3)},
                 {database_msg::kAddWriters,    std::bind(&crud::handle_add_writers,    this, _1, _2, _3)},
                 {database_msg::kRemoveWriters, std::bind(&crud::handle_remove_writers, this, _1, _2, _3)},
                 {database_msg::kQuickRead,     std::bind(&crud::handle_read,           this, _1, _2, _3)}}
{
}


void
crud::start()
{
    std::call_once(this->start_once,
        [this]()
        {
            this->subscription_manager->start();
        });
}


void
crud::handle_request(const bzn::caller_id_t& caller_id, const database_msg& request, const std::shared_ptr<bzn::session_base> session)
{
    if (auto it = this->message_handlers.find(request.msg_case()); it != this->message_handlers.end())
    {
        LOG(debug) << "processing message: " << uint32_t(request.msg_case());

        it->second(caller_id, request, session);

        return;
    }

    LOG(error) << "unknown request: " << uint32_t(request.msg_case());
}


void
crud::send_response(const database_msg& request, const bzn::storage_result result,
    database_response&& response, std::shared_ptr<bzn::session_base>& session)
{
    *response.mutable_header() = request.header();

    if (result != bzn::storage_result::ok)
    {
        if (bzn::storage_result_msg.count(result))
        {
            response.mutable_error()->set_message(bzn::storage_result_msg.at(result));
        }
        else
        {
            LOG(error) << "unknown error code: " << uint32_t(result);
        }
    }

    bzn_envelope env;
    env.set_database_response(response.SerializeAsString());
    env.set_sender("placeholder for daemon's uuid"); // TODO
    // TODO: crypto

    if (session)
    {
        session->send_message(std::make_shared<std::string>(env.SerializeAsString()), false);
    }
    else
    {
        LOG(warning) << "session not set - response for the " << uint32_t(request.msg_case()) << " operation not sent via session";
    }

    if (this->node && !response.header().point_of_contact().empty())
    {
        try
        {
            this->node->send_message(response.header().point_of_contact(), std::make_shared<bzn_envelope>(env), false);
        }
        catch(const std::runtime_error& err)
        {
            LOG(error) << err.what();
        }
    }
    else
    {
        LOG(error) << "Unable to send response for the " << uint32_t(request.msg_case()) << " operation to point of contact - node not set in crud module";
    }
}

void
crud::handle_create(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    bzn::storage_result result{bzn::storage_result::db_not_found};

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    auto [db_exists, perms] = this->get_database_permissions(request.header().db_uuid());

    if (db_exists)
    {
        if (!this->is_caller_a_writer(caller_id, perms))
        {
            result = bzn::storage_result::access_denied;
        }
        else
        {
            result = this->storage->create(request.header().db_uuid(), request.create().key(), request.create().value());

            if (result == bzn::storage_result::ok)
            {
                this->subscription_manager->inspect_commit(request);
            }
        }
    }

    this->send_response(request, result, database_response(), session);
}


void
crud::handle_read(const bzn::caller_id_t& /*caller_id*/, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    const bzn::key_t key = (request.msg_case() == database_msg::kRead) ? request.read().key() : request.quick_read().key();

    const auto result = this->storage->read(request.header().db_uuid(), key);

    database_response response;

    if (result)
    {
        response.mutable_read()->set_key(key);
        response.mutable_read()->set_value(*result);
    }

    this->send_response(request, (result) ? bzn::storage_result::ok : bzn::storage_result::not_found, std::move(response), session);
}


void
crud::handle_update(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    bzn::storage_result result{bzn::storage_result::db_not_found};

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    const auto [db_exists, perms] = this->get_database_permissions(request.header().db_uuid());

    if (db_exists)
    {
        if (!this->is_caller_a_writer(caller_id, perms))
        {
            result = bzn::storage_result::access_denied;
        }
        else
        {
            result = this->storage->update(request.header().db_uuid(), request.update().key(), request.update().value());

            if (result == bzn::storage_result::ok)
            {
                this->subscription_manager->inspect_commit(request);
            }
        }
    }

    this->send_response(request, result, database_response(), session);
}


void
crud::handle_delete(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    bzn::storage_result result{bzn::storage_result::db_not_found};

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    const auto [db_exists, perms] = this->get_database_permissions(request.header().db_uuid());

    if (db_exists)
    {
        if (!this->is_caller_a_writer(caller_id, perms))
        {
            result = bzn::storage_result::access_denied;
        }
        else
        {
            result = this->storage->remove(request.header().db_uuid(), request.delete_().key());

            if (result == bzn::storage_result::ok)
            {
                this->subscription_manager->inspect_commit(request);
            }
        }
    }

    this->send_response(request, result, database_response(), session);
}


void
crud::handle_has(const bzn::caller_id_t& /*caller_id*/, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    database_response response;

    response.mutable_has()->set_key(request.has().key());
    response.mutable_has()->set_has(this->storage->has(request.header().db_uuid(), request.has().key()));

    this->send_response(request, bzn::storage_result::ok, std::move(response), session);
}


void
crud::handle_keys(const bzn::caller_id_t& /*caller_id*/, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    const auto keys = this->storage->get_keys(request.header().db_uuid());

    database_response response;
    response.mutable_keys();

    for (const auto& key : keys)
    {
        response.mutable_keys()->add_keys(key);
    }

    this->send_response(request, bzn::storage_result::ok, std::move(response), session);
}


void
crud::handle_size(const bzn::caller_id_t& /*caller_id*/, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    const auto [keys, size] = this->storage->get_size(request.header().db_uuid());

    database_response response;

    response.mutable_size()->set_keys(keys);
    response.mutable_size()->set_bytes(size);

    this->send_response(request, bzn::storage_result::ok, std::move(response), session);
}


void
crud::handle_subscribe(const bzn::caller_id_t& /*caller_id*/, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    if (session)
    {
        database_response response;

        this->subscription_manager->subscribe(request.header().db_uuid(), request.subscribe().key(),
            request.header().nonce(), response, session);

        this->send_response(request, bzn::storage_result::ok, std::move(response), session);

        return;
    }

    LOG(warning) << "session no longer available. SUBSCRIBE not executed.";
}


void
crud::handle_unsubscribe(const bzn::caller_id_t& /*caller_id*/, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    if (session)
    {
        database_response response;

        this->subscription_manager->unsubscribe(request.header().db_uuid(), request.unsubscribe().key(),
            request.unsubscribe().nonce(), response, session);

        this->send_response(request, bzn::storage_result::ok, std::move(response), session);

        return;
    }

    // subscription manager will cleanup stale sessions...
    LOG(warning) << "session no longer available. UNSUBSCRIBE not executed.";
}


void
crud::handle_create_db(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    bzn::storage_result result;

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (this->storage->has(PERMISSION_UUID, request.header().db_uuid()))
    {
        result = bzn::storage_result::db_exists;
    }
    else
    {
        result = this->storage->create(PERMISSION_UUID, request.header().db_uuid(), this->create_permission_data(caller_id));
    }

    this->send_response(request, result, database_response(), session);
}


void
crud::handle_delete_db(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    bzn::storage_result result{bzn::storage_result::db_not_found};

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    const auto [db_exists, perms] = this->get_database_permissions(request.header().db_uuid());

    if (db_exists)
    {
        if (!this->is_caller_owner(caller_id, perms))
        {
            result = bzn::storage_result::access_denied;
        }
        else
        {
            result = this->storage->remove(PERMISSION_UUID, request.header().db_uuid());

            this->storage->remove(request.header().db_uuid());
        }
    }

    this->send_response(request, result, database_response(), session);
}


void
crud::handle_has_db(const bzn::caller_id_t& /*caller_id*/, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    database_response response;

    response.mutable_has_db()->set_uuid(request.header().db_uuid());
    response.mutable_has_db()->set_has(this->storage->has(PERMISSION_UUID, request.header().db_uuid()));

    this->send_response(request, bzn::storage_result::ok, std::move(response), session);
}


void
crud::handle_writers(const bzn::caller_id_t& /*caller_id*/, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
     bzn::storage_result result{bzn::storage_result::not_found};
     std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

     const auto [db_exists, perms] = this->get_database_permissions(request.header().db_uuid());

     if (db_exists)
     {
         database_response resp;

         resp.mutable_writers()->set_owner(perms[OWNER_KEY].asString());

         for(const auto& writer : perms[WRITERS_KEY])
         {
             resp.mutable_writers()->add_writers(writer.asString());
         }

         this->send_response(request, bzn::storage_result::ok, std::move(resp), session);

         return;
     }
     else
     {
         this->send_response(request, result, database_response(), session);
     }
}


void
crud::handle_add_writers(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    bzn::storage_result result{bzn::storage_result::db_not_found};

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    auto [db_exists, perms] = this->get_database_permissions(request.header().db_uuid());

    if (db_exists)
    {
        if (!this->is_caller_owner(caller_id, perms))
        {
            result = bzn::storage_result::access_denied;
        }
        else
        {
            this->add_writers(request, perms);

            LOG(debug) << "updating db perms: " << perms.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

            if (result = this->storage->update(PERMISSION_UUID, request.header().db_uuid(), perms.toStyledString()); result != bzn::storage_result::ok)
            {
                throw std::runtime_error("Failed to update database permissions: " + bzn::storage_result_msg.at(result));
            }
        }
    }

    this->send_response(request, result, database_response(), session);
}


void
crud::handle_remove_writers(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session)
{
    bzn::storage_result result{bzn::storage_result::db_not_found};

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    auto [db_exists, perms] = this->get_database_permissions(request.header().db_uuid());

    if (db_exists)
    {
        if (!this->is_caller_owner(caller_id, perms))
        {
            result = bzn::storage_result::access_denied;
        }
        else
        {
            this->remove_writers(request, perms);

            LOG(debug) << "updating db perms: " << perms.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

            if (result = this->storage->update(PERMISSION_UUID, request.header().db_uuid(), perms.toStyledString()); result != bzn::storage_result::ok)
            {
                throw std::runtime_error("Failed to update database permissions: " + bzn::storage_result_msg.at(result));
            }
        }
    }

    this->send_response(request, result, database_response(), session);
}


std::pair<bool, Json::Value>
crud::get_database_permissions(const bzn::uuid_t& uuid) const
{
    // does the db exist?
    if (this->storage->has(PERMISSION_UUID, uuid))
    {
        auto perms_data = this->storage->read(PERMISSION_UUID, uuid);

        if (!perms_data)
        {
            throw std::runtime_error("Failed to read database permission data!");
        }

        Json::Reader reader;
        Json::Value json;

        if (!reader.parse(*perms_data, json))
        {
            throw std::runtime_error("Failed to parse database json permission data: " + reader.getFormattedErrorMessages());
        }

        return {true, json};
    }

    return {false, Json::Value()};
}


bzn::value_t
crud::create_permission_data(const bzn::caller_id_t& caller_id) const
{
    Json::Value json;

    json[OWNER_KEY] = boost::trim_copy(caller_id);
    json[WRITERS_KEY] = Json::Value(Json::arrayValue);

    LOG(debug) << "created db perms: " << json.toStyledString();

    return json.toStyledString();
}


bool
crud::is_caller_owner(const bzn::caller_id_t& caller_id, const Json::Value& perms) const
{
    return perms[OWNER_KEY] == boost::trim_copy(caller_id);
}


bool
crud::is_caller_a_writer(const bzn::caller_id_t& caller_id, const Json::Value& perms) const
{
    for(const auto& writer_id : perms[WRITERS_KEY])
    {
        if (writer_id == boost::trim_copy(caller_id))
        {
            return true;
        }
    }

    return this->is_caller_owner(caller_id, perms);
}


void
crud::add_writers(const database_msg& request, Json::Value& perms)
{
    const std::string owner = perms[OWNER_KEY].asString();

    std::set<std::string> current_writers;

    for (auto& writer : perms[WRITERS_KEY])
    {
        current_writers.emplace(writer.asString());
    }

    for (const auto& writer : request.add_writers().writers())
    {
        // owner never should be in the writers list...
        if (writer != owner)
        {
            current_writers.insert(writer);
        }
    }

    perms[WRITERS_KEY].clear();

    for (auto&& writer : current_writers)
    {
        perms[WRITERS_KEY].append(std::move(writer));
    }
}


void
crud::remove_writers(const database_msg& request, Json::Value& perms)
{
    std::set<std::string> current_writers;

    for (auto& writer : perms[WRITERS_KEY])
    {
        current_writers.emplace(writer.asString());
    }

    for (const auto& writer : request.remove_writers().writers())
    {
        current_writers.erase(writer);
    }

    perms[WRITERS_KEY].clear();

    for (auto&& writer : current_writers)
    {
        perms[WRITERS_KEY].append(std::move(writer));
    }
}


bool
crud::save_state()
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    return this->storage->create_snapshot();
}


std::shared_ptr<std::string>
crud::get_saved_state()
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    return this->storage->get_snapshot();
}


bool
crud::load_state(const std::string& state)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    return this->storage->load_snapshot(state);
}
