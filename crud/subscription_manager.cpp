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

#include <crud/subscription_manager.hpp>
#include <algorithm>

using namespace bzn;

namespace
{
    const std::chrono::seconds DEFAULT_DEAD_SESSION_CHECK{15};
}


subscription_manager::subscription_manager(std::shared_ptr<bzn::asio::io_context_base> io_context)
    : purge_timer(io_context->make_unique_steady_timer())
{
}


void
subscription_manager::start()
{
    std::call_once(this->start_once,
        [this]()
        {
            this->purge_timer->expires_from_now(DEFAULT_DEAD_SESSION_CHECK);
            this->purge_timer->async_wait(std::bind(&subscription_manager::purge_closed_sessions, shared_from_this(), std::placeholders::_1));
        });

}


void
subscription_manager::subscribe(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session)
{
    LOG(debug) << "session [" << session->get_session_id() << "] subscription request: " << uuid << ":" << transaction_id << ":" << key;

    std::lock_guard<std::mutex> lock(this->subscribers_lock);

    if (auto database_it = this->subscribers.find(uuid); database_it != this->subscribers.end())
    {
        if (auto key_it = database_it->second.find(key); key_it == database_it->second.end())
        {
            // add new key and subscriber...
            database_it->second[key][session->get_session_id()][transaction_id] = std::move(session);

            return;
        }
        else
        {
            // find existing session...
            auto session_it = database_it->second[key].find(session->get_session_id());

            if (session_it == database_it->second[key].end() || session_it->second.find(transaction_id) == session_it->second.end())
            {
                // add new key and subscriber...
                database_it->second[key][session->get_session_id()][transaction_id] = std::move(session);

                return;
            }
        }

        // session already subscribed to this key...
        response.mutable_error()->set_message(MSG_DUPLICATE_SUB);

        LOG(debug) << "session [" << session->get_session_id() << "] has already subscribed to: " << uuid << ":" << transaction_id << ":" << key;

        return;
    }

    // create a new entry...
    this->subscribers[uuid][key][session->get_session_id()][transaction_id] = std::move(session);
}


void
subscription_manager::unsubscribe(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session)
{
    LOG(debug) << "session [" << session->get_session_id() << "] unsubscribe request: " << uuid << ":" << key << ":" << transaction_id;

    std::lock_guard<std::mutex> lock(this->subscribers_lock);

    auto database_it = this->subscribers.find(uuid);

    if (database_it == this->subscribers.end())
    {
        response.mutable_error()->set_message(MSG_INVALID_UUID);

        LOG(debug) << "session [" << session->get_session_id() << "] unknown database & key: " << uuid << ":" << key << ":" << transaction_id;

        return;
    }

    auto key_it = database_it->second.find(key);

    if (key_it == database_it->second.end())
    {
        response.mutable_error()->set_message(MSG_INVALID_KEY);

        LOG(debug) << "session [" << session->get_session_id() << "] unknown key: " << uuid << ":" << key << ":" << transaction_id;

        return;
    }

    // check to see if session is already in the list...
    if (auto session_it = database_it->second[key].find(session->get_session_id()); session_it == key_it->second.end())
    {
        response.mutable_error()->set_message(MSG_INVALID_SUB);

        LOG(debug) << "session [" << session->get_session_id() << "] not subscribed to: " << uuid << ":" << key << ":" <<  transaction_id;

        return;
    }
    else
    {
        // remove subscription...
        if (auto sub_it = session_it->second.find(transaction_id); sub_it != session_it->second.end())
        {
            session_it->second.erase(sub_it);

            return;
        }

        response.mutable_error()->set_message(MSG_INVALID_SUB);
        LOG(debug) << "session [" << session->get_session_id() << "] not subscribed to: " << uuid << ":" << key << ":" <<  transaction_id;
    }
}


void
subscription_manager::notify_sessions(const bzn::uuid_t& uuid, const bool update, const bzn::key_t& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(this->subscribers_lock);

    if (auto database_it = this->subscribers.find(uuid); database_it != this->subscribers.end())
    {
        if (auto key_it = database_it->second.find(key); key_it != database_it->second.end())
        {
            for (const auto& sessions : key_it->second)
            {
                for (const auto& subscription : sessions.second)
                {
                    if (auto session_shared_ptr = subscription.second.lock())
                    {
                        database_response resp;

                        resp.mutable_header()->set_db_uuid(uuid);
                        resp.mutable_header()->set_transaction_id(subscription.first);
                        resp.mutable_subscription_update()->set_key(key);

                        if (!value.empty())
                        {
                            resp.mutable_subscription_update()->set_value(value);
                        }

                        if (update)
                        {
                            resp.mutable_subscription_update()->set_operation(database_subscription_update::UPDATE);
                        }
                        else
                        {
                            resp.mutable_subscription_update()->set_operation(database_subscription_update::DELETE);
                        }

                        LOG(debug) << "notifying session [" << session_shared_ptr->get_session_id() << "] : " << uuid
                                   << ":" << key << ":" << subscription.first << ":" << value.substr(0, MAX_MESSAGE_SIZE);

                        session_shared_ptr->send_datagram(std::make_shared<std::string>(resp.SerializeAsString()));
                    }
                }
            }
        }
    }
}


void
subscription_manager::inspect_commit(const database_msg& msg)
{
    switch (msg.msg_case())
    {
        case database_msg::kCreate:
            this->notify_sessions(msg.header().db_uuid(), true, msg.create().key(), msg.create().value());
            break;

        case database_msg::kUpdate:
            this->notify_sessions(msg.header().db_uuid(), true, msg.update().key(), msg.update().value());
            break;

        case database_msg::kDelete:
            this->notify_sessions(msg.header().db_uuid(), false, msg.delete_().key(), "");
            break;

        default:
            // nothing to do...
            break;
    }
}


void
subscription_manager::purge_closed_sessions(const boost::system::error_code& ec)
{
    if (!ec)
    {
        std::lock_guard<std::mutex> lock(this->subscribers_lock);

        auto database_it = this->subscribers.begin();

        while (database_it != this->subscribers.end())
        {
            size_t purged{};

            auto key_it = database_it->second.begin();

            while (key_it != database_it->second.end())
            {
                auto session_it = key_it->second.begin();

                while (session_it != key_it->second.end())
                {
                    auto subscribers_it = session_it->second.begin();

                    while (subscribers_it != session_it->second.end())
                    {
                        if (auto session_shared_ptr = subscribers_it->second.lock())
                        {
                            ++subscribers_it;
                            continue;
                        }

                        LOG(debug) << "purged closed session [" << session_it->first << "] for: " << database_it->first;

                        ++purged;

                        subscribers_it = session_it->second.erase(subscribers_it);
                    }

                    if (session_it->second.empty())
                    {
                        session_it = key_it->second.erase(session_it);
                        continue;
                    }

                    ++session_it;
                }

                if (key_it->second.empty())
                {
                    key_it = database_it->second.erase(key_it);
                    continue;
                }

                ++key_it;
            }

            if (purged)
            {
                LOG(info) << "purged " << purged << " closed sessions for database: " << database_it->first;
            }

            if (database_it->second.empty())
            {
                database_it = this->subscribers.erase(database_it);
                continue;
            }

            ++database_it;
        }

        // reschedule...
        this->purge_timer->expires_from_now(DEFAULT_DEAD_SESSION_CHECK);
        this->purge_timer->async_wait(std::bind(&subscription_manager::purge_closed_sessions, shared_from_this(), std::placeholders::_1));
    }
}
