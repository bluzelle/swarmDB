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

#include <include/bluzelle.hpp>
#include <storage/storage_base.hpp>
#include <options/options_base.hpp>
#include <rocksdb/db.h>
#include <shared_mutex>


namespace bzn
{
    class rocksdb_storage : public bzn::storage_base
    {
    public:
        rocksdb_storage(const std::string& state_dir, const std::string& db_name, const bzn::uuid_t& uuid);

        bzn::storage_result create(const bzn::uuid_t& uuid, const bzn::key_t& key, const bzn::value_t& value) override;

        std::optional<bzn::value_t> read(const bzn::uuid_t& uuid, const bzn::key_t& key) override;

        bzn::storage_result update(const bzn::uuid_t& uuid, const bzn::key_t& key, const bzn::value_t& value) override;

        bzn::storage_result remove(const bzn::uuid_t& uuid, const bzn::key_t& key) override;

        std::vector<bzn::key_t> get_keys(const bzn::uuid_t& uuid) override;

        bool has(const bzn::uuid_t& uuid, const  bzn::key_t& key) override;

        std::pair<std::size_t, std::size_t> get_size(const bzn::uuid_t& uuid) override;

        std::optional<std::size_t> get_key_size(const bzn::uuid_t& uuid, const bzn::key_t& key) override;

        bzn::storage_result remove(const bzn::uuid_t& uuid) override;

        bool create_snapshot() override;

        std::shared_ptr<std::string> get_snapshot() override;

        bool load_snapshot(const std::string& data) override;

        void remove_range(const bzn::uuid_t& uuid, const bzn::key_t& first, const bzn::key_t& last) override;

        std::vector<std::pair<bzn::key_t, bzn::value_t>> read_if(const bzn::uuid_t& uuid,
            const bzn::key_t& first, const bzn::key_t& last,
            std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate = std::nullopt) override;

        std::vector<bzn::key_t> get_keys_if(const bzn::uuid_t& uuid,
            const bzn::key_t& first, const bzn::key_t& last,
            std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate = std::nullopt) override;

    private:
        void open();

        // metadata....
        void update_metadata_size(const bzn::uuid_t& uuid, const bzn::key_t& metadata_key, const bzn::key_t& key,uint32_t size);
        void delete_metadata_size(const bzn::uuid_t& uuid, const bzn::key_t& meta_key, const bzn::key_t& key);
        uint32_t get_metadata_size(const bzn::uuid_t& uuid, const bzn::key_t& meta_key, const bzn::key_t& key);

        const std::string db_path;
        const std::string snapshot_file;

        std::unique_ptr<rocksdb::DB> db;

        bool has_priv(const bzn::uuid_t& uuid, const bzn::key_t& key);

        std::shared_mutex lock; // for multi-reader and single writer access

        void do_if(const bzn::uuid_t& uuid, const bzn::key_t& first, const bzn::key_t& last,
            std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>> predicate,
            std::function<void(const bzn::key_t&, const bzn::value_t&)> action);

        void db_flush() const;
    };

} // bzn
