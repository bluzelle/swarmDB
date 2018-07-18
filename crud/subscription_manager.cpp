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

    template<typename T1, typename T2>
    auto find_session(T1& sessions, T2& session)
    {
        return std::find_if(sessions.begin(), sessions.end(),
            [&session](const auto& entry)
            {
                // get the shared ptr if it's still valid...
                if (auto session_shared_ptr = entry.second.lock())
                {
                    return session_shared_ptr == session;
                }
                return false;
            });
    }
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
    LOG(debug) << "received subscription request: " << uuid << ":" << transaction_id << ":" << key;

    std::lock_guard<std::mutex> lock(this->subscribers_lock);

    if (auto database_it = this->subscribers.find(uuid); database_it != this->subscribers.end())
    {
        if (auto key_it = database_it->second.find(key); key_it == database_it->second.end())
        {
            // append new key and subscriber...
            database_it->second[key].emplace_back(std::make_pair(transaction_id, std::move(session)));

            return;
        }
        else
        {
            if (find_session(key_it->second, session) == key_it->second.end())
            {
                key_it->second.emplace_back(std::make_pair(transaction_id, std::move(session)));

                return;
            }
        }

        // session already subscribed to this key...
        response.mutable_error()->set_message(MSG_DUPLICATE_SUB);

        LOG(debug) << "session (" << session.get() << ") has already subscribed to: " << uuid << ":" << key;

        return;
    }

    // create a new entry...
    this->subscribers[uuid][key].emplace_back(std::make_pair(transaction_id, std::move(session)));
}


void
subscription_manager::unsubscribe(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session)
{
    LOG(debug) << "received unsubscription request: " << uuid << ":" << transaction_id << ":" << key;

    std::lock_guard<std::mutex> lock(this->subscribers_lock);

    auto database_it = this->subscribers.find(uuid);

    if (database_it == this->subscribers.end())
    {
        response.mutable_error()->set_message(MSG_INVALID_UUID);

        LOG(debug) << "unknown database & key: " << uuid << ":" << transaction_id << ":" << key;

        return;
    }

    auto key_it = database_it->second.find(key);

    if (key_it == database_it->second.end())
    {
        response.mutable_error()->set_message(MSG_INVALID_KEY);

        LOG(debug) << "unknown key: " << uuid << ":" << transaction_id << ":" << key;

        return;
    }

    // check to see if session is already in the list...
    if (auto session_it = find_session(key_it->second, session); session_it == key_it->second.end())
    {
        response.mutable_error()->set_message(MSG_INVALID_SUB);

        LOG(debug) << "session not subscribed to: " << uuid << ":" << transaction_id << ":" <<  key;

        return;
    }
    else
    {
        // remove session...
        key_it->second.erase(session_it);
    }
}


void
subscription_manager::notify_sessions(const bzn::uuid_t& uuid, const bzn::key_t& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(this->subscribers_lock);

    if (auto database_it = this->subscribers.find(uuid); database_it != this->subscribers.end())
    {
        if (auto key_it = database_it->second.find(key); key_it != database_it->second.end())
        {
            for (const auto& session_pair : key_it->second)
            {
                if (auto session_shared_ptr = session_pair.second.lock())
                {
                    database_response resp;

                    resp.mutable_header()->set_db_uuid(uuid);
                    resp.mutable_header()->set_transaction_id(session_pair.first);

                    resp.mutable_subscription_update()->set_key(key);
                    resp.mutable_subscription_update()->set_value(value);

                    LOG(debug) << "notifying session (" << session_shared_ptr.get() << ") : " << uuid
                               << ":" << session_pair.first << ":" << key << ":" << value.substr(0, MAX_MESSAGE_SIZE);

                    session_shared_ptr->send_datagram(std::make_shared<std::string>(resp.SerializeAsString()));
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
            this->notify_sessions(msg.header().db_uuid(), msg.create().key(), msg.create().value());
            break;

        case database_msg::kUpdate:
            this->notify_sessions(msg.header().db_uuid(), msg.update().key(), msg.update().value());
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

        for (auto& database : this->subscribers)
        {
            size_t purged{};

            for (auto& key : database.second)
            {
                std::remove_if(key.second.begin(), key.second.end(),
                    [&](const auto& entry)
                    {
                        if (auto session_shared_ptr = entry.second.lock())
                        {
                            return false;
                        }

                        LOG(debug) << "purged closed session for: " << database.first << ":" << entry.first << ":" << key.first;

                        ++purged;

                        return true;
                    });
            }

            LOG(info) << "purged " << purged << " closed sessions for database: " << database.first;
        }

        // reschedule...
        this->purge_timer->expires_from_now(DEFAULT_DEAD_SESSION_CHECK);
        this->purge_timer->async_wait(std::bind(&subscription_manager::purge_closed_sessions, shared_from_this(), std::placeholders::_1));
    }
}
