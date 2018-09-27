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
#include <storage/rocksdb_storage.hpp>
#include <mocks/mock_node_base.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <cstdlib>

using namespace ::testing;

namespace
{
    const bzn::uuid_t NODE_UUID = "d1e04722-41f0-4c43-a6c0-86a9e62a88e3";
    const bzn::uuid_t USER_UUID = "4bba2aeb-44fe-441e-bb6b-8817561eb716";
    const std::string KEY = "bluzelle.json";
    std::string value = "ewogICJsaXN0ZW5lcl9hZGRyZXNzIiA6ICIxMjcuMC4wLjEiLAogICJsaXN0ZW5lcl9wb3J0IiA6"
                        "IDQ5MTUyLAogICJldGhlcmV1bSIgOiAiMHgwMDZlYWU3MjA3NzQ0OWNhY2E5MTA3OGVmNzg1NTJj"
                        "MGNkOWJjZThmIgp9Cg==";

    boost::random::mt19937 gen;

    std::string
    generate_test_string(int n = 100)
    {
        std::string test_data;
        test_data.resize(n);

        boost::random::uniform_int_distribution<> dist('a', 'z');
        for (auto& c : test_data)
        {
            c = (char) dist(gen);
        }
        return test_data;
    }

    // factory functions...
    template<class T>
    std::shared_ptr<bzn::storage_base> create_storage();

    template<>
    std::shared_ptr<bzn::storage_base> create_storage<bzn::mem_storage>()
    {
        return std::make_shared<bzn::mem_storage>();
    }

    template<>
    std::shared_ptr<bzn::storage_base> create_storage<bzn::rocksdb_storage>()
    {
        return std::make_shared<bzn::rocksdb_storage>("./", NODE_UUID);
    }
}


template<typename T>
class storageTest : public Test
{
public:
    storageTest()
    {
        // just to be safe...
        system(std::string("rm -r -f " + NODE_UUID).c_str());
        this->storage = create_storage<T>();
    }

    ~storageTest()
    {
        // For each run, the database has to be cleared out...
        this->storage.reset();
        system(std::string("rm -r -f " + NODE_UUID).c_str());
    }

    std::shared_ptr<bzn::storage_base> storage;
};

using Implementations = Types<bzn::mem_storage, bzn::rocksdb_storage>;

TYPED_TEST_CASE(storageTest, Implementations);


TYPED_TEST(storageTest, test_that_storage_can_create_a_record_and_read_the_same_record)
{
    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(USER_UUID, KEY, value));

    const auto returned_record = this->storage->read(USER_UUID, KEY);

    EXPECT_EQ((*returned_record).size(), this->storage->get_size(USER_UUID));

    // add another one...
    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(USER_UUID, "another_key", value));
    EXPECT_EQ(value.size()*2, this->storage->get_size(USER_UUID));

    EXPECT_EQ(*returned_record, value);
}


TYPED_TEST(storageTest, test_that_storage_fails_to_create_a_record_that_already_exists)
{
    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(USER_UUID, KEY, value));

    EXPECT_EQ(bzn::storage_base::result::exists, this->storage->create(USER_UUID, KEY, value));
}


TYPED_TEST(storageTest, test_that_attempting_to_read_a_record_from_storage_that_does_not_exist_returns_null)
{
    EXPECT_EQ(std::nullopt, this->storage->read(USER_UUID, "nokey"));
}


TYPED_TEST(storageTest, test_that_storage_can_update_an_existing_record)
{
    const std::string updated_value = "I have changed the value of the text";

    // try updating a record that does not exist
    EXPECT_EQ(bzn::storage_base::result::not_found, this->storage->update(USER_UUID, KEY, updated_value));

    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(USER_UUID, KEY, value));

    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->update(USER_UUID, KEY, updated_value));

    const auto returned_record = this->storage->read(USER_UUID, KEY);

    EXPECT_EQ(*returned_record, updated_value);
}


TYPED_TEST(storageTest, test_that_storage_can_delete_a_record)
{
    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(USER_UUID, KEY, value));

    const auto returned_record = this->storage->read(USER_UUID, KEY);

    EXPECT_EQ(*returned_record, value);

    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->remove(USER_UUID, KEY));

    EXPECT_EQ(this->storage->read(USER_UUID, KEY), std::nullopt);

    EXPECT_EQ(bzn::storage_base::result::not_found, this->storage->remove(USER_UUID, KEY));
}


TYPED_TEST(storageTest, test_get_keys_returns_all_keys)
{
    const bzn::uuid_t user_0{"b9dc2595-15ee-435a-8af7-7cafc132f527"};
    const bzn::uuid_t user_1{"fa82925e-4657-11e8-842f-0ed5f89f718b"};
    const bzn::uuid_t user_2{"f3f72dd5-efcb-4db9-b0a1-47f7891ffdef"};

    std::vector<std::string> user_keys;

    std::string key;

    for(int i=0; i<100; ++i)
    {
        key = "key";
        key.append(std::to_string(i));
        user_keys.emplace_back(key);

        this->storage->create(user_0, key, generate_test_string());
        this->storage->create(user_1, key, generate_test_string());
        this->storage->create(user_2, key, generate_test_string());
    }

    std::vector<std::string> user_0_keys = this->storage->get_keys(user_0);
    std::vector<std::string> user_1_keys = this->storage->get_keys(user_1);
    std::vector<std::string> user_2_keys = this->storage->get_keys(user_2);

    EXPECT_EQ(user_0_keys.size(), user_keys.size());
    EXPECT_EQ(user_1_keys.size(), user_keys.size());
    EXPECT_EQ(user_2_keys.size(), user_keys.size());


    std::sort(user_keys.begin(), user_keys.end());
    std::sort(user_0_keys.begin(), user_0_keys.end());
    std::sort(user_1_keys.begin(), user_1_keys.end());
    std::sort(user_2_keys.begin(), user_2_keys.end());

    if (user_0_keys.size() >0)
    {
        EXPECT_TRUE(std::equal(user_keys.begin(), user_keys.end(), user_0_keys.begin() ));
        EXPECT_TRUE(std::equal(user_keys.begin(), user_keys.end(), user_1_keys.begin() ));
        EXPECT_TRUE(std::equal(user_keys.begin(), user_keys.end(), user_2_keys.begin() ));
    }
}


TYPED_TEST(storageTest, test_has_returns_true_if_key_exists_false_otherwise)
{
    const bzn::uuid_t user_0{"b9dc2595-15ee-435a-8af7-7cafc132f527"};
    const bzn::uuid_t user_1{"fa82925e-4657-11e8-842f-0ed5f89f718b"};
    const bzn::uuid_t user_2{"f3f72dd5-efcb-4db9-b0a1-47f7891ffdef"};

    std::vector<std::string> user_keys;

    std::string key;

    for(int i=0; i<100; ++i)
    {
        key = "key";
        key.append(std::to_string(i));
        user_keys.emplace_back(key);

        this->storage->create(user_0, key, generate_test_string());
        this->storage->create(user_1, key, generate_test_string());
        this->storage->create(user_2, key, generate_test_string());
    }

    EXPECT_TRUE(this->storage->has(user_0, "key0"));
    EXPECT_FALSE(this->storage->has(user_0, "notkey0"));

    EXPECT_TRUE(this->storage->has(user_1, "key1"));
    EXPECT_FALSE(this->storage->has(user_1, "notkey"));

    EXPECT_TRUE(this->storage->has(user_2, "key2"));
    EXPECT_FALSE(this->storage->has(user_2, "notkey"));
}


TYPED_TEST(storageTest, test_that_storage_fails_to_create_a_value_that_exceeds_the_size_limit)
{
    std::string value{""};
    value.resize(bzn::MAX_VALUE_SIZE+1, 'c');
    EXPECT_EQ(bzn::storage_base::result::value_too_large, this->storage->create(USER_UUID, KEY, value));
    EXPECT_EQ(std::nullopt, this->storage->read(USER_UUID, KEY));
}


TYPED_TEST(storageTest, test_that_storage_fails_to_update_with_a_value_that_exceeds_the_size_limit)
{
    std::string expected_value{"gooddata"};
    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(USER_UUID, KEY, expected_value));
    auto actual_record = this->storage->read(USER_UUID, KEY);
    EXPECT_EQ(expected_value, actual_record);

    std::string bad_value{""};
    bad_value.resize(bzn::MAX_VALUE_SIZE+1, 'c');
    EXPECT_EQ(bzn::storage_base::result::value_too_large, this->storage->update(USER_UUID, KEY, bad_value));
    EXPECT_EQ(expected_value, *this->storage->read(USER_UUID, KEY));
}
