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

#include <include/bluzelle.hpp>
#include <storage/storage_base.hpp>
#include <node/node_base.hpp>
#include <unordered_map>
#include <shared_mutex>


namespace bzn
{
    class storage : public bzn::storage_base, public std::enable_shared_from_this<storage>
    {
    public:
        storage(std::shared_ptr<bzn::node_base> node);

        void create(const bzn::uuid_t& uuid, const std::string& key, const std::string& value) override;

        std::shared_ptr<bzn::storage_base::record> read(const bzn::uuid_t& uuid, const std::string& key) override;

        void update(const bzn::uuid_t& uuid, const std::string& key, const std::string& value) override;

        void remove(const bzn::uuid_t& uuid, const std::string& key) override;

        void start() override;

    private:
        void priv_msg_handler(const bzn::message& msg, std::shared_ptr<bzn::session_base> session);

        std::unordered_map<std::string, std::shared_ptr<bzn::storage_base::record>> kv_store;
        std::shared_mutex write_lock;

        std::shared_ptr<bzn::node_base> node;
    };

} // bzn