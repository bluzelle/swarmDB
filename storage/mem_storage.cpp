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

#include <storage/mem_storage.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <sstream>

using namespace bzn;


storage_base::result
mem_storage::create(const bzn::uuid_t& uuid, const std::string& key, const std::string& value)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (value.size() > bzn::MAX_VALUE_SIZE)
    {
        return storage_base::result::value_too_large;
    }

    if (key.size() > bzn::MAX_KEY_SIZE)
    {
        return storage_base::result::key_too_large;
    }

    if (auto search = this->kv_store.find(uuid); search != this->kv_store.end())
    {
        if (search->second.find(key)!= search->second.end() )
        {
            return storage_base::result::exists;
        }
    }

    auto& inner_db =  this->kv_store[uuid];

    if (inner_db.find(key) == inner_db.end())
    {
        // todo: test if insert failed?
        inner_db.insert(std::make_pair(key,value));
    }
    else
    {
        return storage_base::result::exists;
    }

    return storage_base::result::ok;
}


std::optional<bzn::value_t>
mem_storage::read(const bzn::uuid_t& uuid, const std::string& key)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    auto search = this->kv_store.find(uuid);

    if (search == this->kv_store.end())
    {
        return std::nullopt;
    }

    // we have the db, let's see if the key exists
    auto& inner_db = search->second;
    auto inner_search = inner_db.find(key);
    if (inner_search == inner_db.end())
    {
        return std::nullopt;
    }
    return inner_search->second;
}


storage_base::result
mem_storage::update(const bzn::uuid_t& uuid, const std::string& key, const std::string& value)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (value.size() > bzn::MAX_VALUE_SIZE)
    {
        return storage_base::result::value_too_large;
    }

    auto search = this->kv_store.find(uuid);

    if (search == this->kv_store.end())
    {
        return bzn::storage_base::result::not_found;
    }


    // we have the db, let's see if the key exists
    auto& inner_db = search->second;
    auto inner_search = inner_db.find(key);

    if (inner_search == inner_db.end())
    {
        return bzn::storage_base::result::not_found;
    }

    inner_search->second = value;
    return storage_base::result::ok;
}


storage_base::result
mem_storage::remove(const bzn::uuid_t& uuid, const std::string& key)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    auto search = this->kv_store.find(uuid);

    if (search == this->kv_store.end())
    {
        return storage_base::result::not_found;
    }

    auto record = search->second.find(key);

    if (record == search->second.end())
    {
        return storage_base::result::not_found;
    }

    search->second.erase(record);
    return storage_base::result::ok;
}


std::vector<std::string>
mem_storage::get_keys(const bzn::uuid_t& uuid)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    auto inner_db = this->kv_store.find(uuid);

    if (inner_db == this->kv_store.end())
    {
        return {};
    }

    std::vector<std::string> keys;
    for (const auto& p : inner_db->second)
    {
        keys.emplace_back(p.first);
    }

    return keys;
}


bool
mem_storage::has(const bzn::uuid_t& uuid, const std::string& key)
{
    const auto v = this->get_keys(uuid);
    return std::find(v.begin(), v.end(), key) != v.end();
}


// todo: optimize! Track size as it grows and not iterate over every value!
std::pair<std::size_t, std::size_t>
mem_storage::get_size(const bzn::uuid_t& uuid)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    auto it = this->kv_store.find(uuid);

    if (it == this->kv_store.end())
    {
        // database not found...
        return std::make_pair(0,0);
    }

    std::size_t size{};
    std::size_t keys{};

    for (const auto& record : it->second)
    {
        ++keys;
        size += record.second.size();
    }

    return std::make_pair(keys, size);
}


storage_base::result
mem_storage::remove(const bzn::uuid_t& uuid)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (auto it = this->kv_store.find(uuid); it != this->kv_store.end())
    {
        this->kv_store.erase(it);

        return storage_base::result::ok;
    }

    return storage_base::result::not_found;
}

bool
mem_storage::create_snapshot()
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    try
    {
        std::stringstream strm;
        boost::archive::text_oarchive archive(strm);
        archive << this->kv_store;
        this->latest_snapshot = std::make_shared<std::string>(strm.str());

        return true;
    }
    catch(std::exception &e)
    {
        LOG(error) << "Exception creating snapshot: " << e.what();
    }
    return false;
}

std::shared_ptr<std::string>
mem_storage::get_snapshot()
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access
    return this->latest_snapshot;
}

bool
mem_storage::load_snapshot(const std::string& data)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for write access

    try
    {
        std::stringstream strm(data);
        boost::archive::text_iarchive archive(strm);
        archive >> this->kv_store;
        this->latest_snapshot = std::make_shared<std::string>(data);

        return true;
    }
    catch(std::exception &e)
    {
        LOG(error) << "Exception loading snapshot: " << e.what();
    }
    return false;
}