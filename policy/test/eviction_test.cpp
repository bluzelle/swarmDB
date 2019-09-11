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
#include <googletest/src/googletest/include/gtest/gtest.h>
#include <policy/volatile_ttl.hpp>
#include <crud/crud.hpp>
#include <mocks/mock_node_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_subscription_manager_base.hpp>
#include <mocks/mock_boost_asio_beast.hpp>

using namespace ::testing;

namespace
{
    const char* DB_UUID{"e6d16cab-cd0f-4af1-b45d-16de9bc2af5d"};
    //const char* CALLER_UUID{"9558fa26-ba03-4e24-98bc-1f2f562c4598"};
    // 2b3c2de4-5c14-4842-8284-b67eb09ec61d
    // bf90f8d1-cb27-4217-a5c5-85c8e6565a25
    // 557bab19-8df2-4731-b74e-c2516e1f580f

    const std::string TTL_UUID{"TTL"};

    database_msg
    make_create_request(const bzn::uuid_t& db_uuid, const bzn::key_t key, const bzn::value_t& value)
    {
        database_msg request;

        auto header = new database_header();
        header->set_db_uuid(db_uuid);

        auto create = new database_create();
        create->set_key(key);
        create->set_value(value);

        request.set_allocated_create(create);
        request.set_allocated_header(header);

        return request;
    }


    database_msg
    make_update_request(const bzn::uuid_t& db_uuid, const bzn::key_t key, const bzn::value_t& value)
    {
        database_msg request;

        auto header = new database_header();
        header->set_db_uuid(db_uuid);

        auto update = new database_update();
        update->set_key(key);
        update->set_value(value);

        request.set_allocated_update(update);
        request.set_allocated_header(header);

        return request;
    }


    // TODO: This is duplication of private code in storage, it may be a good idea to find a way
    // to dry this up
    bzn::key_t
    generate_expire_key(const bzn::uuid_t& uuid, const bzn::key_t& key)
    {
        Json::Value value;

        value["uuid"] = uuid;
        value["key"] = key;

        return value.toStyledString();
    }


    size_t
    insert_test_values(std::shared_ptr<bzn::storage_base> storage, const bzn::uuid_t& db_uuid, size_t number_of_items, size_t value_size=128)
    {
        for (size_t i{0}; i<number_of_items; ++i)
        {
            std::string key{"key" + std::to_string(i)};
            storage->create( db_uuid, key, std::string(value_size, 'B'));
        }

        return storage->get_size(db_uuid).second;
    }


    void
    insert_ttl_values(std::shared_ptr<bzn::storage_base> storage, const bzn::uuid_t& db_uuid, const std::vector<bzn::key_t> keys)
    {
        size_t i{0};
        for (const auto& key : keys)
        {
            const auto expires = boost::lexical_cast<std::string>(
                    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 1024 + i * 1024);
            ++i;
            storage->create(TTL_UUID, generate_expire_key(db_uuid,key), expires);
        }
    }
}

namespace bzn
{
    TEST(policy_test, test_that_volatile_ttl_ignores_keys_with_no_ttl)
    {
        std::shared_ptr<bzn::storage_base> storage{std::make_shared<bzn::mem_storage>()};
        const auto MAX_DB_SIZE{insert_test_values(storage, DB_UUID, 10) + 5};
        policy::volatile_ttl sut(storage);

        // performing a create on a db with no TTLs should return no keys to delete
        {
            const auto request{make_create_request(DB_UUID, "testvalue", std::string(256, 'C'))};
            const auto keys_to_delete{sut.keys_to_evict(request, MAX_DB_SIZE)};
            EXPECT_EQ(size_t(0), keys_to_delete.size());
        }

        // Update should behave the same way
        {
            const auto request{make_update_request(DB_UUID, "testvalue", std::string(256,'C'))};
            const auto keys_to_delete{sut.keys_to_evict(request, MAX_DB_SIZE)};
            EXPECT_EQ( size_t(0), keys_to_delete.size());
        }
    }

    TEST(policy_test, test_that_volatile_ttl_returns_only_keys_with_ttl)
    {
        std::shared_ptr<bzn::storage_base> storage{std::make_shared<bzn::mem_storage>()};

        const size_t NUMBER_OF_ITEMS{10};
        const size_t VALUE_SIZE{128};
        const auto MAX_STORAGE{insert_test_values(storage, DB_UUID, NUMBER_OF_ITEMS, VALUE_SIZE)};

        const bzn::key_t TEST_KEY{"key1"};
        std::vector<bzn::key_t> keys{TEST_KEY};
        insert_ttl_values(storage, DB_UUID, keys);

        policy::volatile_ttl sut(storage);

        const bzn::value_t TEST_VALUE{std::string(64, 'B')};

        // evict the one ttl key to make room for the created key value pair
        {
            auto request{make_create_request(DB_UUID, "KEY_CREATE", TEST_VALUE)};
            const auto keys_to_delete{sut.keys_to_evict(request, MAX_STORAGE)};
            EXPECT_EQ(size_t(1), keys_to_delete.size());
            EXPECT_EQ(TEST_KEY, *keys_to_delete.begin());
        }

        // evict the one ttl key to make room for the updated key value pair
        {
            auto request{make_update_request(DB_UUID, "KEY_UPDATE", TEST_VALUE)};
            const auto keys_to_delete{sut.keys_to_evict(request, MAX_STORAGE)};
            EXPECT_EQ(size_t(1), keys_to_delete.size());
            EXPECT_EQ(TEST_KEY, *keys_to_delete.begin());
        }
    }

    TEST(policy_test, test_that_volatile_ttl_fails_if_there_are_not_enough_ttl_values_to_remove)
    {
        std::shared_ptr<bzn::storage_base> storage{std::make_shared<bzn::mem_storage>()};

        const size_t NUMBER_OF_ITEMS{10};

        const size_t VALUE_SIZE{128};
        const auto MAX_STORAGE{insert_test_values(storage, DB_UUID, NUMBER_OF_ITEMS, VALUE_SIZE) + 5};

        // set only one TTL value
        const bzn::key_t TEST_KEY{"key1"};
        std::vector<bzn::key_t> keys{TEST_KEY};
        insert_ttl_values(storage, DB_UUID, keys);

        policy::volatile_ttl sut(storage);

        // try creating a large value, this must fail as there is only one small key value pair with a TTL
        // (return no keys to evict)
        {
            auto request{make_create_request(DB_UUID, "KEY_CREATE", std::string(2 * VALUE_SIZE, 'B'))};
            const auto keys_to_delete{sut.keys_to_evict(request, MAX_STORAGE)};
            EXPECT_EQ(size_t(0), keys_to_delete.size());
        }

        // try updating an existing value by doubling its size. This must fail (ibid)
        {
            auto request{make_update_request(DB_UUID, "KEY_CREATE", std::string(2 * VALUE_SIZE, 'B'))};
            const auto keys_to_delete{sut.keys_to_evict(request, MAX_STORAGE)};
            EXPECT_EQ(size_t(0), keys_to_delete.size());
        }

        // try updating the key with a TTL, this must fail (the only key with a ttl is the key we are updating.)
        {
            auto request{make_update_request(DB_UUID, TEST_KEY, std::string(2 * VALUE_SIZE, 'B'))};
            const auto keys_to_delete{sut.keys_to_evict(request, MAX_STORAGE)};
            EXPECT_EQ(size_t(0), keys_to_delete.size());
        }
    }
}
