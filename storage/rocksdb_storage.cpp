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
#include <rocksdb/db_dump_tool.h>
#include <thread>

using namespace bzn;

namespace
{
    inline bzn::key_t generate_key(const bzn::uuid_t& uuid, const bzn::key_t& key)
    {
        return uuid+key;
    }
}


rocksdb_storage::rocksdb_storage(const std::string& state_dir, const std::string& db_name, const bzn::uuid_t& uuid)
    : db_path(boost::filesystem::path(state_dir).append(uuid).append(db_name).string())
    , snapshot_file(boost::filesystem::path(state_dir).append(uuid).append("SNAPSHOT." + db_name).string())
{
    this->open();
}


void
rocksdb_storage::open()
{
    rocksdb::Options options;

    options.IncreaseParallelism(std::thread::hardware_concurrency());
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;

    rocksdb::DB* rocksdb;

    boost::filesystem::create_directories(db_path);

    LOG(info) << "database path: " << db_path;

    rocksdb::Status s = rocksdb::DB::Open(options, db_path,& rocksdb);

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
    rocksdb::DumpOptions dump_options;

    dump_options.db_path = this->db_path;
    dump_options.dump_location = this->snapshot_file;
    dump_options.anonymous = true;

    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    return rocksdb::DbDumpTool().Run(dump_options);
}


std::shared_ptr<std::string>
rocksdb_storage::get_snapshot()
{
    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access

    std::stringstream snapshot;

    // check if file exists...
    try
    {
        std::ifstream s(this->snapshot_file);
        snapshot << s.rdbuf();
    }
    catch (std::exception& ex)
    {
        LOG(error) << "Exception reading snapshot: " << ex.what();

        return {};
    }

    return std::make_shared<std::string>(snapshot.str());
}


bool
rocksdb_storage::load_snapshot(const std::string& data)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    const std::string tmp_snapshot(this->snapshot_file + ".tmp");

    try
    {
        std::ofstream snapshot(tmp_snapshot);
        snapshot << data;
    }
    catch (std::exception& ex)
    {
        LOG(error) << "saving snapshot failed: " << ex.what();

        return false;
    }

    // bring down the database...
    this->db.reset();

    // move current database out of the way...
    const std::string tmp_path(this->db_path + ".tmp");

    boost::system::error_code ec;
    boost::filesystem::rename(this->db_path, tmp_path, ec);

    if (ec)
    {
        LOG(error) << "creating temporary db backup failed: " << ec.message();

        // bring db back online...
        this->open();

        return false;
    }

    rocksdb::UndumpOptions undump_options;

    undump_options.db_path = this->db_path;
    undump_options.dump_location = tmp_snapshot;
    undump_options.compact_db = true;

    if (rocksdb::DbUndumpTool().Run(undump_options))
    {
        boost::system::error_code ec;
        boost::filesystem::remove_all(tmp_path, ec);
        boost::filesystem::remove(this->snapshot_file);
        boost::filesystem::rename(tmp_snapshot, this->snapshot_file);

        if (ec)
        {
            LOG(error) << "failed to remove temporary db backup: " << ec.message();
        }

        // bring db back online...
        this->open();

        return true;
    }

    LOG(error) << "failed to load snapshot";

    // any exceptions will be fatal...
    boost::filesystem::remove_all(this->db_path);
    boost::filesystem::rename(tmp_path, this->db_path);
    boost::filesystem::remove(tmp_snapshot);

    // bring db back online...
    this->open();

    return false;
}


void
rocksdb_storage::remove_range(const bzn::uuid_t& uuid, const std::string& first, const std::string& last)
{
    std::lock_guard<std::shared_mutex> lock(this->lock); // lock for write access

    std::vector<bzn::key_t> v;

    auto begin_str = generate_key(uuid, first);
    rocksdb::Slice begin(begin_str);

    auto end_str = generate_key(uuid, last);
    rocksdb::Slice end(end_str);

    rocksdb::WriteOptions write_options;
    write_options.sync = true;

    this->db->DeleteRange(write_options, nullptr, begin, end);
}


void
rocksdb_storage::do_if(const bzn::uuid_t& uuid, const std::string& first, const std::string& last
    , std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate
    , std::function<void(const bzn::key_t&, const bzn::value_t&)> action)
{
    const auto start_key = uuid + first;
    const auto end_key = last.empty() ? "" : uuid + last;

    std::shared_lock<std::shared_mutex> lock(this->lock); // lock for read access
    std::unique_ptr<rocksdb::Iterator> iter(this->db->NewIterator(rocksdb::ReadOptions()));

    for (iter->Seek(start_key); iter->Valid() && iter->key().starts_with(uuid) && iter->key().ToString() >= start_key
        && (end_key.empty() || iter->key().ToString() < end_key); iter->Next())
    {
        if (!predicate || (*predicate)(iter->key().ToString().substr(uuid.size()), iter->value().ToString()))
        {
            action(iter->key().ToString().substr(uuid.size()), iter->value().ToString());
        }
    }
}


std::vector<std::pair<bzn::key_t, bzn::value_t>>
rocksdb_storage::read_if(const bzn::uuid_t& uuid, const std::string& first, const std::string& last
    , std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate)
{
    std::vector<std::pair<bzn::key_t, bzn::value_t>> matches;

    this->do_if(uuid, first, last, predicate, [&](auto key, auto value)
    {
        matches.emplace_back(std::make_pair(key, value));
    });

    return matches;
}


std::vector<bzn::key_t>
rocksdb_storage::get_keys_if(const bzn::uuid_t& uuid, const std::string& first, const std::string& last
    , std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate)
{
    std::vector<std::string> keys;
    this->do_if(uuid, first, last, predicate, [&](auto key, auto /*value*/)
    {
        keys.emplace_back(key);
    });

    return keys;
}
