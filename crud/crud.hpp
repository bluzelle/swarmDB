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

#include <crud/crud_base.hpp>
#include <crud/subscription_manager_base.hpp>
#include <include/bluzelle.hpp>
#include <include/boost_asio_beast.hpp>
#include <node/node_base.hpp>
#include <pbft/pbft_base.hpp>
#include <storage/storage_base.hpp>
#include <shared_mutex>


namespace bzn
{
    class crud final : public bzn::crud_base, public std::enable_shared_from_this<crud>
    {
    public:
        crud(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::storage_base> storage, std::shared_ptr<bzn::subscription_manager_base> subscription_manager,
            std::shared_ptr<bzn::node_base> node);

        void handle_request(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session) override;

        void start(std::shared_ptr<bzn::pbft_base> pbft) override;

        bool save_state() override;

        std::shared_ptr<std::string> get_saved_state() override;

        bool load_state(const std::string& state) override;

    private:

        void handle_create_db(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_delete_db(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_has_db(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_create(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_read(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_update(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_delete(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_ttl(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_persist(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_has(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_keys(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_size(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_subscribe(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_unsubscribe(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_writers(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_add_writers(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);
        void handle_remove_writers(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session);

        void send_response(const database_msg& request, bzn::storage_result result, database_response&& response, std::shared_ptr<bzn::session_base>& session);

        // permission...
        std::pair<bool, Json::Value> get_database_permissions(const bzn::uuid_t& uuid) const;
        bzn::value_t create_permission_data(const bzn::caller_id_t& caller_id) const;
        bool is_caller_owner(const bzn::caller_id_t& caller_id, const Json::Value& perms) const;
        bool is_caller_a_writer(const bzn::caller_id_t& caller_id, const Json::Value& perms) const;
        void add_writers(const database_msg& request, Json::Value& perms);
        void remove_writers(const database_msg& request, Json::Value& perms);

        // expiration...
        void check_key_expiration(const boost::system::error_code& ec);
        bool expired(const bzn::uuid_t& uuid, const bzn::key_t& key);
        void update_expiration_entry(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t expire);
        void remove_expiration_entry(const bzn::key_t& generated_key);
        void flush_expiration_entries(const bzn::uuid_t& uuid);
        std::optional<uint64_t> get_ttl(const bzn::uuid_t& uuid, const bzn::key_t& key) const;

        std::shared_ptr<bzn::storage_base> storage;
        std::shared_ptr<bzn::subscription_manager_base> subscription_manager;
        std::shared_ptr<bzn::node_base> node;
        std::shared_ptr<bzn::pbft_base> pbft; // required for expiration
        std::unique_ptr<bzn::asio::steady_timer_base> expire_timer;

        using message_handler_t = std::function<void(const bzn::caller_id_t& caller_id, const database_msg& request, std::shared_ptr<bzn::session_base> session)>;
        std::unordered_map<database_msg::MsgCase, message_handler_t> message_handlers;

        std::once_flag start_once;
        std::shared_mutex lock; // for multi-reader and single writer access
    };

} // namespace bzn
