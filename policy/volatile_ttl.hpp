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

#include <policy/eviction_policy_base.hpp>

namespace bzn::policy
{
    struct ttl_item;

    class volatile_ttl : public eviction_policy_base
    {
    public:
        volatile_ttl(std::shared_ptr<bzn::storage_base> storage) : eviction_policy_base(storage)
        {
        }

        std::set<bzn::key_t> keys_to_evict(const database_msg &request, size_t max_size) override;

    private:
        std::vector<std::string> filter_ttls(const std::vector<std::string>& ttls, const bzn::uuid_t& db_uuid, const bzn::key_t& ignore_key);

        std::vector<ttl_item> get_ttl_items(const std::string &db_uuid, std::vector<std::string> &our_ttls) const;


        // TODO: This is a duplicate of a private crud method, it would be nice to find a way to dry this up,
        // maybe move this function into a utility module
        bzn::key_t generate_expire_key(const bzn::uuid_t& uuid, const bzn::key_t& key) const
        {
            Json::Value value;

            value["uuid"] = uuid;
            value["key"] = key;

            return value.toStyledString();
        }
    };
}
