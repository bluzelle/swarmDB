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

#include <storage/storage_base.hpp>
#include <gmock/gmock.h>

namespace bzn {

    class mock_storage_base : public storage_base {
    public:
        MOCK_METHOD3(create,
            bzn::storage_result(const bzn::uuid_t& uuid, const std::string& key, const std::string& value));
        MOCK_METHOD2(read,
            std::optional<bzn::value_t> (const bzn::uuid_t& uuid, const std::string& key));
        MOCK_METHOD3(update,
            bzn::storage_result(const bzn::uuid_t& uuid, const std::string& key, const std::string& value));
        MOCK_METHOD2(remove,
            bzn::storage_result(const bzn::uuid_t& uuid, const std::string& key));
        MOCK_METHOD0(start,
            bzn::storage_result());
        MOCK_METHOD1(save,
            bzn::storage_result(const std::string& path));
        MOCK_METHOD1(load,
            bzn::storage_result(const std::string& path));
        MOCK_METHOD1(error_msg,
            std::string(bzn::storage_result error_id));
        MOCK_METHOD1(get_keys,
            std::vector<std::string>(const bzn::uuid_t& uuid));
        MOCK_METHOD2(has,
            bool(const bzn::uuid_t& uuid, const std::string& key));
        MOCK_METHOD1(get_size,
            std::pair<std::size_t, std::size_t>(const bzn::uuid_t& uuid));
        MOCK_METHOD2(get_key_size,
            std::optional<std::size_t>(const bzn::uuid_t& uuid, const bzn::key_t& key));
        MOCK_METHOD1(remove,
            bzn::storage_result(const bzn::uuid_t& uuid));
        MOCK_METHOD0(create_snapshot,
            bool());
        MOCK_METHOD0(get_snapshot,
            std::shared_ptr<std::string>());
        MOCK_METHOD1(load_snapshot,
            bool(const std::string&));
        MOCK_METHOD3(remove_range,
            void(const bzn::uuid_t& uuid, const std::string&, const std::string&));
        MOCK_METHOD4(get_keys_if,
            std::vector<bzn::key_t>(const bzn::uuid_t&, const std::string&
            , const std::string&, std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>>));
        MOCK_METHOD4(read_if,
            std::vector<std::pair<bzn::key_t, bzn::value_t>>(const bzn::uuid_t&, const std::string&
            , const std::string&, std::optional<std::function<bool(const bzn::key_t&, const bzn::value_t&)>>));
    };

}  // namespace bzn
