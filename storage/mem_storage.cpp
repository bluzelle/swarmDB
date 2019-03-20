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
#include <boost/serialization/map.hpp>
#include <sstream>

using namespace bzn;


bzn::storage_result
mem_storage::create(const bzn::uuid_t& uuid, const std::string& key, const std::string& value)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (value.size() > bzn::MAX_VALUE_SIZE)
    {
        return bzn::storage_result::value_too_large;
    }

    if (key.size() > bzn::MAX_KEY_SIZE)
    {
        return bzn::storage_result::key_too_large;
    }

    if (auto search = this->kv_store.find(uuid); search != this->kv_store.end())
    {
        if (search->second.second.find(key)!= search->second.second.end() )
        {
            return bzn::storage_result::exists;
        }
    }

    auto& inner_db =  this->kv_store[uuid];

    if (inner_db.second.find(key) == inner_db.second.end())
    {
        inner_db.first += value.size();
        inner_db.second.insert(std::make_pair(key,value));
    }
    else
    {
        return bzn::storage_result::exists;
    }

    return bzn::storage_result::ok;
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
    auto inner_search = inner_db.second.find(key);
    if (inner_search == inner_db.second.end())
    {
        return std::nullopt;
    }
    return inner_search->second;
}


bzn::storage_result
mem_storage::update(const bzn::uuid_t& uuid, const std::string& key, const std::string& value)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (value.size() > bzn::MAX_VALUE_SIZE)
    {
        return bzn::storage_result::value_too_large;
    }

    auto search = this->kv_store.find(uuid);

    if (search == this->kv_store.end())
    {
        return bzn::storage_result::not_found;
    }

    // we have the db, let's see if the key exists
    auto& inner_db = search->second;
    auto inner_search = inner_db.second.find(key);

    if (inner_search == inner_db.second.end())
    {
        return bzn::storage_result::not_found;
    }

    inner_db.first -= inner_search->second.size();
    inner_search->second = value;
    inner_db.first += inner_search->second.size();

    return bzn::storage_result::ok;
}


bzn::storage_result
mem_storage::remove(const bzn::uuid_t& uuid, const std::string& key)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    auto search = this->kv_store.find(uuid);

    if (search == this->kv_store.end())
    {
        return bzn::storage_result::not_found;
    }

    auto record = search->second.second.find(key);

    if (record == search->second.second.end())
    {
        return bzn::storage_result::not_found;
    }

    search->second.first -= record->second.size();
    search->second.second.erase(record);
    return bzn::storage_result::ok;
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
    for (const auto& p : inner_db->second.second)
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

    return std::make_pair(it->second.second.size(), it->second.first);
}


bzn::storage_result
mem_storage::remove(const bzn::uuid_t& uuid)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (auto it = this->kv_store.find(uuid); it != this->kv_store.end())
    {
        this->kv_store.erase(it);

        return bzn::storage_result::ok;
    }

    return bzn::storage_result::not_found;
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
    catch (std::exception& ex)
    {
        LOG(error) << "Exception creating snapshot: " << ex.what();
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
    catch (std::exception& ex)
    {
        LOG(error) << "Exception loading snapshot: " << ex.what();
    }

    return false;
}


void
mem_storage::remove_range(const bzn::uuid_t& uuid, const std::string& first, const std::string& last)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    auto inner_db = this->kv_store.find(uuid);
    if (inner_db != this->kv_store.end())
    {
        auto match = inner_db->second.second.lower_bound(first);
        auto end = inner_db->second.second.lower_bound(last);
        while (match != end)
        {
            inner_db->second.first -= match->second.size();
            match = inner_db->second.second.erase(match);
        }
    }
}


void
mem_storage::do_if(const bzn::uuid_t& uuid, const std::string& first, const std::string& last,
    std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate,
    std::function<void(const bzn::key_t&, const bzn::value_t&)> action)
{
    if (!last.empty() && last <= first)
    {
        return;
    }

    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    auto inner_db = this->kv_store.find(uuid);
    if (inner_db != this->kv_store.end())
    {
        auto end_it = last.empty() ? inner_db->second.second.end() : inner_db->second.second.lower_bound(last);
        for (auto it = inner_db->second.second.lower_bound(first); it != inner_db->second.second.end() && it != end_it; it++)
        {
            if (!predicate || (*predicate)(it->first, it->second))
            {
                action(it->first, it->second);
            }
        }
    }
}


std::vector<std::pair<bzn::key_t, bzn::value_t>>
mem_storage::read_if(const bzn::uuid_t& uuid, const std::string& first, const std::string& last,
    std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate)
{
    std::vector<std::pair<bzn::key_t, bzn::value_t>> matches;

    this->do_if(uuid, first, last, predicate, [&](auto key, auto value)
    {
        matches.emplace_back(std::make_pair(key, value));
    });

    return matches;
}


std::vector<bzn::key_t>
mem_storage::get_keys_if(const bzn::uuid_t& uuid, const std::string& first, const std::string& last
    , std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate)
{
    std::vector<std::string> keys;
    this->do_if(uuid, first, last, predicate, [&](auto key, auto /*value*/)
    {
        keys.emplace_back(key);
    });

    return keys;
}
