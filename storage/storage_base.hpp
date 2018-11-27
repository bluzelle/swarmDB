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
#include <optional>
#include <unordered_map>
#include <vector>


namespace bzn
{
    const size_t MAX_KEY_SIZE   = 4096;
    const size_t MAX_VALUE_SIZE = 256000;

    enum class storage_result : uint8_t
    {
        ok=0,
        not_found,
        exists,
        not_saved,
        value_too_large,
        key_too_large,
        db_not_found,
        access_denied
    };

    const std::unordered_map<storage_result, const std::string> storage_result_msg{
        {storage_result::ok,              "OK"},
        {storage_result::not_found,       "RECORD_NOT_FOUND"},
        {storage_result::exists,          "RECORD_EXISTS"},
        {storage_result::not_saved,       "NOT_SAVED"},
        {storage_result::value_too_large, "VALUE_SIZE_TOO_LARGE"},
        {storage_result::key_too_large,   "KEY_SIZE_TOO_LARGE"},
        {storage_result::db_not_found,    "DATABASE_NOT_FOUND"},
        {storage_result::access_denied,   "ACCESS_DENIED"}};


    class storage_base
    {
    public:
        virtual ~storage_base() = default;

        virtual bzn::storage_result create(const bzn::uuid_t& uuid, const std::string& key, const std::string& value) = 0;

        virtual std::optional<bzn::value_t> read(const bzn::uuid_t& uuid, const std::string& key) = 0;

        virtual bzn::storage_result update(const bzn::uuid_t& uuid, const std::string& key, const std::string& value) = 0;

        virtual bzn::storage_result remove(const bzn::uuid_t& uuid, const std::string& key) = 0;

        virtual std::vector<bzn::key_t> get_keys(const bzn::uuid_t& uuid) = 0;
        
        virtual bool has(const bzn::uuid_t& uuid, const  std::string& key) = 0;

        virtual std::pair<std::size_t, std::size_t> get_size(const bzn::uuid_t& uuid) = 0;

        virtual bzn::storage_result remove(const bzn::uuid_t& uuid) = 0;

        virtual bool create_snapshot() = 0;

        virtual std::shared_ptr<std::string> get_snapshot() = 0;

        virtual bool load_snapshot(const std::string& data) = 0;
    };

} // bzn
