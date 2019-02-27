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

#include <pbft/test/pbft_test_common.hpp>
#include <pbft/pbft_persistent_state.hpp>
#include <storage/mem_storage.hpp>
#include <storage/rocksdb_storage.hpp>

using namespace ::testing;

namespace
{
    const bzn::uuid_t NODE_UUID = "d1e04722-41f0-4c43-a6c0-86a9e62a88e3";
}
namespace bzn
{
    class persistent_state_test : public Test
    {
    public:
        std::shared_ptr<bzn::storage_base> storage = std::make_shared<bzn::mem_storage>();
    };

    TEST_F(persistent_state_test, test_assignment)
    {
        persistent<uint64_t> value(this->storage, uint64_t{}, "value");
        value = 10u;

        persistent<uint64_t> value2(this->storage, uint64_t{}, "value");
        EXPECT_EQ(value2.value(), 10u);
    }

    TEST_F(persistent_state_test, uninitialized_map_generates_exception)
    {
        std::map<uint64_t, persistent<std::string>> m;
        EXPECT_THROW(m.insert({1u, {this->storage, "one", std::string("m"), static_cast<uint64_t>(1)}}), std::runtime_error);
    }

    TEST_F(persistent_state_test, test_initialize)
    {
        std::map<uint64_t, persistent<std::string>> m;
        persistent<std::string>::init_kv_container<uint64_t>(this->storage, "m", m);

        m.insert({1u, {this->storage, "one", std::string("m"), static_cast<uint64_t>(1)}});
        m.insert({2u, {this->storage, "two", std::string("m"), static_cast<uint64_t>(2)}});

        std::map<uint64_t, persistent<std::string>> m2;
        persistent<std::string>::init_kv_container<uint64_t>(this->storage, "m", m2);

        EXPECT_EQ(m, m2);
    }

    TEST_F(persistent_state_test, test_initialize_nested)
    {
        std::map<uint64_t, std::map<uint64_t, persistent<std::string>>> m;
        persistent<std::string>::init_kv_container2<uint64_t, uint64_t>(this->storage, "m", m);

        m[1].insert({3u, {this->storage, "one", std::string("m"), static_cast<uint64_t>(3), static_cast<uint64_t>(1)}});
        m[2].insert({4u, {this->storage, "two", std::string("m"), static_cast<uint64_t>(4), static_cast<uint64_t>(2)}});

        std::map<uint64_t, std::map<uint64_t, persistent<std::string>>> m2;
        persistent<std::string>::init_kv_container2<uint64_t, uint64_t>(this->storage, "m", m2);

        EXPECT_EQ(m, m2);
    }

    TEST_F(persistent_state_test, test_escaping)
    {
        std::map<uint64_t, std::map<std::string, persistent<std::string>>> m;
        persistent<std::string>::init_kv_container2<std::string, uint64_t>(this->storage
            , std::string{"m"} + ESCAPE_1 + "1", m);

        // m[1].insert({"test/one", {this->storage, "one", std::string("m/1"), std::string{"test/one"}, static_cast<uint64_t>(1)}});
        m[1].insert({std::string{"test"} + ESCAPE_1 + "one", {this->storage, "one", std::string{"m"} + ESCAPE_1 + "1"
            , std::string{"test"} + ESCAPE_1 + "one", static_cast<uint64_t>(1)}});
        m[2].insert({std::string{"test"} + ESCAPE_1 + "two", {this->storage, "two", std::string{"m"} + ESCAPE_1 + "1"
            , std::string{"test"} + ESCAPE_1 + "two", static_cast<uint64_t>(2)}});

        std::map<uint64_t, std::map<std::string, persistent<std::string>>> m2;
        persistent<std::string>::init_kv_container2<std::string, uint64_t>(this->storage
            , std::string{"m"} + ESCAPE_1 + "1", m2);

        EXPECT_EQ(m, m2);

        auto key1 = persistent<std::string>::generate_key(std::string{"hello"} + ESCAPE_1, std::string{"world"});
        auto key2 = persistent<std::string>::generate_key(std::string{"hello"}, std::string{ESCAPE_1} + "world");
        EXPECT_NE(key1, key2);

        std::string subkey1{ESCAPE_1};
        std::string subkey2{std::string{ESCAPE_2} + "x"};
        auto key_res = persistent<std::string>::generate_key(subkey1, subkey2);
        auto extracted = persistent<std::string>::extract_subkeys<std::string
            , std::string>(key_res.substr(SEPARATOR.size()));
        EXPECT_EQ(subkey1, std::get<0>(extracted));
        EXPECT_EQ(subkey2, std::get<1>(extracted));
    }

    TEST_F(persistent_state_test, test_physical_storage)
    {
        system(std::string("rm -r -f " + NODE_UUID).c_str());

        {
            auto phys_storage = std::make_shared<bzn::rocksdb_storage>("./", "utest", NODE_UUID);
            persistent<uint64_t> int_value(phys_storage, 10u, "int_value");
            persistent<std::string> str_value(phys_storage, "my_value", "str_value");

            std::map<uint64_t, persistent<std::string>> map_value;
            persistent<std::string>::init_kv_container<uint64_t>(phys_storage, "map_value", map_value);
            map_value[1] = {phys_storage, "one", "map_value", uint64_t{1u}};
            map_value[2] = {phys_storage, "two", "map_value", uint64_t{2u}};
            map_value[10] = {phys_storage, "ten", "map_value", uint64_t{10u}};
        }

        {
            auto phys_storage = std::make_shared<bzn::rocksdb_storage>("./", "utest", NODE_UUID);
            persistent<uint64_t> int_value(phys_storage, 0, "int_value");
            EXPECT_EQ(int_value.value(), 10u);
            persistent<std::string> str_value(phys_storage, "", "str_value");
            EXPECT_EQ(str_value.value(), "my_value");

            std::map<uint64_t, persistent<std::string>> map_value;
            persistent<std::string>::init_kv_container<uint64_t>(phys_storage, "map_value", map_value);
            EXPECT_EQ(map_value[1].value(), "one");
            EXPECT_EQ(map_value[2].value(), "two");
            EXPECT_EQ(map_value[10].value(), "ten");

        }

        system(std::string("rm -r -f " + NODE_UUID).c_str());
    }

    TEST_F(persistent_state_test, test_conversions)
    {
        std::string str1{"This is a test"};
        std::string str1_1{persistent<std::string>::to_string(str1)};
        std::string str1_2{persistent<std::string>::from_string(str1)};
        EXPECT_EQ(str1, str1_1);
        EXPECT_EQ(str1, str1_2);

        bzn::log_key_t log_key{1u, 2u};
        std::string log_key_1{persistent<bzn::log_key_t>::to_string(log_key)};
        bzn::log_key_t log_key_2{persistent<bzn::log_key_t>::from_string(log_key_1)};
        EXPECT_EQ(log_key, log_key_2);

        bzn::operation_key_t operation_key{1u, 2u, "hash"};
        std::string operation_key_1{persistent<bzn::operation_key_t>::to_string(operation_key)};
        bzn::operation_key_t operation_key_2{persistent<bzn::operation_key_t>::from_string(operation_key_1)};
        EXPECT_EQ(operation_key, operation_key_2);

        bzn::checkpoint_t checkpoint{1u, "hash"};
        std::string checkpoint_1{persistent<bzn::checkpoint_t>::to_string(checkpoint)};
        bzn::checkpoint_t checkpoint_2{persistent<bzn::checkpoint_t>::from_string(checkpoint_1)};
        EXPECT_EQ(checkpoint, checkpoint_2);
    }

    TEST_F(persistent_state_test, test_forked_alias_generates_exception)
    {
        persistent<std::string> str{this->storage, "test", "test_key"};
        persistent<std::string> str2{this->storage, "test2", "test_key"};
        EXPECT_EQ(str2.value(), "test");
        str2 = "test2";
        EXPECT_THROW(str = "test", std::runtime_error);
    }
}