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

#include <storage/rocksdb_storage.hpp>
#include <boost/filesystem.hpp>
#include <thread>

using namespace bzn;

namespace
{
    inline bzn::key_t generate_key(const bzn::uuid_t& uuid, const bzn::key_t& key)
    {
        return uuid+key;
    }
}


rocksdb_storage::rocksdb_storage(const std::string& state_dir, const bzn::uuid_t& uuid)
{
    rocksdb::Options options;

    options.IncreaseParallelism(std::thread::hardware_concurrency());
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;

    rocksdb::DB* rocksdb;
    rocksdb::Status s = rocksdb::DB::Open(options, boost::filesystem::path(state_dir).append(uuid).string(), &rocksdb);

    if (!s.ok())
    {
        throw std::runtime_error("Could not open database: " + s.ToString());
    }

    this->db.reset(rocksdb);
}


bzn::storage_result
rocksdb_storage::create(const bzn::uuid_t& uuid, const std::string& key, const std::string& value)
{
    if (value.size() > bzn::MAX_VALUE_SIZE)
    {
        return bzn::storage_result::value_too_large;
    }

    if (key.size() > bzn::MAX_KEY_SIZE)
    {
        return bzn::storage_result::key_too_large;
    }

    rocksdb::WriteOptions write_options;
    write_options.sync = true;

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (!this->has_priv(uuid, key))
    {
        auto s = this->db->Put(write_options, generate_key(uuid, key), value);

        if (!s.ok())
        {
            LOG(error) << "save failed: " << uuid << ":" << key << ":" <<
                value.substr(0,MAX_MESSAGE_SIZE) << "... - " << s.ToString();

            return bzn::storage_result::not_saved;
        }

        return bzn::storage_result::ok;
    }

    return bzn::storage_result::exists;
}


std::optional<bzn::value_t>
rocksdb_storage::read(const bzn::uuid_t& uuid, const std::string& key)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    bzn::value_t value;
    auto s = this->db->Get(rocksdb::ReadOptions(), generate_key(uuid, key), &value);

    if (!s.ok())
    {
        return std::nullopt;
    }

    return value;
}


bzn::storage_result
rocksdb_storage::update(const bzn::uuid_t& uuid, const std::string& key, const std::string& value)
{
    if (value.size() > bzn::MAX_VALUE_SIZE)
    {
        return bzn::storage_result::value_too_large;
    }

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (this->has_priv(uuid, key))
    {
        rocksdb::WriteOptions write_options;
        write_options.sync = true;

        auto s = this->db->Put(write_options, generate_key(uuid, key), value);

        if (!s.ok())
        {
            LOG(error) << "update failed: " << uuid << ":" << key << ":" <<
                value.substr(0,MAX_MESSAGE_SIZE) << "... - " << s.ToString();

            return bzn::storage_result::not_saved;
        }

        return bzn::storage_result::ok;
    }

    return bzn::storage_result::not_found;
}


bzn::storage_result
rocksdb_storage::remove(const bzn::uuid_t& uuid, const std::string& key)
{
    rocksdb::WriteOptions write_options;
    write_options.sync = true;

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    if (this->has_priv(uuid, key))
    {
        auto s = this->db->Delete(write_options, generate_key(uuid, key));

        if (!s.ok())
        {
            return bzn::storage_result::not_found;
        }

        return bzn::storage_result::ok;
    }

    return bzn::storage_result::not_found;
}


std::vector<bzn::key_t>
rocksdb_storage::get_keys(const bzn::uuid_t& uuid)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    std::unique_ptr<rocksdb::Iterator> iter(this->db->NewIterator(rocksdb::ReadOptions()));

    std::vector<bzn::key_t> v;
    for (iter->Seek(uuid); iter->Valid() && iter->key().starts_with(uuid); iter->Next())
    {
        v.emplace_back(iter->key().ToString().substr(uuid.size()));
    }

    return v;
}


bool
rocksdb_storage::has(const bzn::uuid_t& uuid, const std::string& key)
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    return this->has_priv(uuid, key);
}


std::pair<std::size_t, std::size_t>
rocksdb_storage::get_size(const bzn::uuid_t& uuid)
{
    std::unique_ptr<rocksdb::Iterator> iter(this->db->NewIterator(rocksdb::ReadOptions()));

    std::size_t size{};
    std::size_t keys{};

    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    for (iter->Seek(uuid); iter->Valid() && iter->key().starts_with(uuid); iter->Next())
    {
        ++keys;
        size += iter->value().size();
    }

    return std::make_pair(keys, size);
}


bzn::storage_result
rocksdb_storage::remove(const bzn::uuid_t& uuid)
{
    rocksdb::WriteOptions write_options;
    write_options.sync = true;

    std::unique_ptr<rocksdb::Iterator> iter(this->db->NewIterator(rocksdb::ReadOptions()));

    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    std::size_t keys_removed{};

    for (iter->Seek(uuid); iter->Valid() && iter->key().starts_with(uuid); iter->Next())
    {
        auto s = this->db->Delete(write_options, iter->key());

        if (!s.ok())
        {
            LOG(error) << "delete failed: " << uuid << ":" <<  s.ToString();
        }

        ++keys_removed;
    }

    return (keys_removed) ? bzn::storage_result::ok : bzn::storage_result::not_found;
}


bool
rocksdb_storage::has_priv(const bzn::uuid_t& uuid, const  std::string& key)
{
    const bzn::key_t has_key = generate_key(uuid, key);

    std::unique_ptr<rocksdb::Iterator> iter(this->db->NewIterator(rocksdb::ReadOptions()));

    for (iter->Seek(uuid); iter->Valid() && iter->key().starts_with(uuid); iter->Next())
    {
        if (iter->key() == has_key)
        {
            return true;
        }
    }

    return false;
}

bool
rocksdb_storage::create_snapshot()
{
    return false;
}

std::shared_ptr<std::string>
rocksdb_storage::get_snapshot()
{
    return nullptr;
}

bool
rocksdb_storage::load_snapshot(const std::string& /*data*/)
{
    return false;
}