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
#include <regex>
#include <boost/range/irange.hpp>

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
        return std::make_shared<bzn::rocksdb_storage>("./", "utest", NODE_UUID);
    }
}


template<typename T>
class storageTest : public Test
{
public:
    storageTest()
    {
        // just to be safe...
        if (system(std::string("rm -r -f " + NODE_UUID).c_str())) {}
        this->storage = create_storage<T>();
    }

    ~storageTest()
    {
        // For each run, the database has to be cleared out...
        this->storage.reset();
        if (system(std::string("rm -r -f " + NODE_UUID).c_str())) {}
    }

    std::shared_ptr<bzn::storage_base> storage;
};

using Implementations = Types<bzn::mem_storage, bzn::rocksdb_storage>;

TYPED_TEST_CASE(storageTest, Implementations);


TYPED_TEST(storageTest, test_that_storage_can_create_a_record_and_read_the_same_record)
{
    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, KEY, value));

    const auto returned_record = this->storage->read(USER_UUID, KEY);

    EXPECT_EQ((*returned_record).size() + KEY.size(), this->storage->get_size(USER_UUID).second);
    EXPECT_EQ(size_t(1), this->storage->get_size(USER_UUID).first);

    // add another one...
    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, "another_key", value));
    EXPECT_EQ(value.size()*2 + KEY.size()+std::string("another_key").size(), this->storage->get_size(USER_UUID).second);
    EXPECT_EQ(size_t(2), this->storage->get_size(USER_UUID).first);

    EXPECT_EQ(*returned_record, value);

    // remove first key
    size_t size = this->storage->get_size(USER_UUID).second;
    this->storage->remove(USER_UUID, KEY);
    EXPECT_EQ(size-(KEY.size()+value.size()), this->storage->get_size(USER_UUID).second);
}


TYPED_TEST(storageTest, test_that_storage_fails_to_create_a_record_that_already_exists)
{
    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, KEY, value));

    EXPECT_EQ(bzn::storage_result::exists, this->storage->create(USER_UUID, KEY, value));
}


TYPED_TEST(storageTest, test_that_attempting_to_read_a_record_from_storage_that_does_not_exist_returns_null)
{
    EXPECT_EQ(std::nullopt, this->storage->read(USER_UUID, "nokey"));
}


TYPED_TEST(storageTest, test_that_storage_can_update_an_existing_record)
{
    const std::string updated_value = "I have changed the value of the text";

    // try updating a record that does not exist
    EXPECT_EQ(bzn::storage_result::not_found, this->storage->update(USER_UUID, KEY, updated_value));

    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, KEY, value));

    EXPECT_EQ(bzn::storage_result::ok, this->storage->update(USER_UUID, KEY, updated_value));

    const auto returned_record = this->storage->read(USER_UUID, KEY);

    EXPECT_EQ(*returned_record, updated_value);
}


TYPED_TEST(storageTest, test_that_storage_can_delete_a_record)
{
    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, KEY, value));

    const auto returned_record = this->storage->read(USER_UUID, KEY);

    EXPECT_EQ(*returned_record, value);

    EXPECT_EQ(bzn::storage_result::ok, this->storage->remove(USER_UUID, KEY));

    EXPECT_EQ(this->storage->read(USER_UUID, KEY), std::nullopt);

    EXPECT_EQ(bzn::storage_result::not_found, this->storage->remove(USER_UUID, KEY));
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
    EXPECT_EQ(bzn::storage_result::value_too_large, this->storage->create(USER_UUID, KEY, value));
    EXPECT_EQ(std::nullopt, this->storage->read(USER_UUID, KEY));
}


TYPED_TEST(storageTest, test_that_storage_fails_to_update_with_a_value_that_exceeds_the_size_limit)
{
    std::string expected_value{"gooddata"};
    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, KEY, expected_value));
    auto actual_record = this->storage->read(USER_UUID, KEY);
    EXPECT_EQ(expected_value, actual_record);

    std::string bad_value{""};
    bad_value.resize(bzn::MAX_VALUE_SIZE+1, 'c');
    EXPECT_EQ(bzn::storage_result::value_too_large, this->storage->update(USER_UUID, KEY, bad_value));
    EXPECT_EQ(expected_value, *this->storage->read(USER_UUID, KEY));
}


TYPED_TEST(storageTest, test_that_storage_can_remove_all_keys_values_associated_with_a_uuid)
{
    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, "key1", ""));
    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, "key2", ""));
    EXPECT_EQ(bzn::storage_result::ok, this->storage->create(USER_UUID, "key3", ""));
    EXPECT_EQ(bzn::storage_result::ok, this->storage->remove(USER_UUID));
    EXPECT_EQ(std::nullopt, this->storage->read(USER_UUID, KEY));
}


TYPED_TEST(storageTest, test_snapshot)
{
    const bzn::uuid_t user_0{"b9dc2595-15ee-435a-8af7-7cafc132f527"};

    this->storage->create(user_0, "key1", "value1");
    EXPECT_TRUE(this->storage->has(user_0, "key1"));
    EXPECT_TRUE(this->storage->create_snapshot());

    this->storage->create(user_0, "key2", "value2");
    this->storage->create(user_0, "key3", "value3");
    EXPECT_TRUE(this->storage->has(user_0, "key2"));
    EXPECT_TRUE(this->storage->has(user_0, "key3"));

    auto state = this->storage->get_snapshot();
    EXPECT_NE(state, nullptr);

    EXPECT_FALSE(this->storage->load_snapshot("aslkdfkslfdk"));
    EXPECT_TRUE(this->storage->load_snapshot(*state));
    EXPECT_TRUE(this->storage->has(user_0, "key1"));
    EXPECT_FALSE(this->storage->has(user_0, "key2"));
    EXPECT_FALSE(this->storage->has(user_0, "key3"));
}

TYPED_TEST(storageTest, test_range_queries)
{
    const bzn::uuid_t user_0{"b9dc2595-15ee-435a-8af7-7cafc132f527"};

    this->storage->create(user_0, "aaa", "value");
    this->storage->create(user_0, "aab", "value");
    this->storage->create(user_0, "abb", "value");
    this->storage->create(user_0, "abc", "value");
    this->storage->create(user_0, "bbc", "value");
    this->storage->create(user_0, "bcc", "value");
    this->storage->create(user_0, "bcd", "value");
    this->storage->create(user_0, "bdd", "value");
    this->storage->create(user_0, "bde", "value");
    EXPECT_EQ(this->storage->get_size(user_0).first, 9u);

    EXPECT_EQ(this->storage->get_keys_if(user_0, "a", "b").size(), 4u);
    EXPECT_EQ(this->storage->read_if(user_0, "b", "c").size(), 5u);
    EXPECT_EQ(this->storage->get_keys_if(user_0, "c", "d").size(), 0u);
    EXPECT_EQ(this->storage->read_if(user_0, "ab", "ac").size(), 2u);

    this->storage->remove_range(user_0, "a", "abb");
    EXPECT_EQ(this->storage->get_size(user_0).first, 7u);

    this->storage->remove_range(user_0, "aa", "bdd");
    EXPECT_EQ(this->storage->get_size(user_0).first, 2u);

    this->storage->remove_range(user_0, "be", "z");
    EXPECT_EQ(this->storage->get_size(user_0).first, 2u);
}

TYPED_TEST(storageTest, test_predicate_queries)
{
    const bzn::uuid_t user_0{"b9dc2595-15ee-435a-8af7-7cafc132f527"};

    for (auto i : boost::irange(0, 100))
    {
        // this line, for some unknown reason, causes the following build error:
        // in function `bzn::pbft_persistent_operation::is_committed() const':
        // undefined reference to `bzn::pbft::honest_majority_size(unsigned long)'
        //auto suffix = (boost::format("%04u") % i).str();

        auto suffix = i < 10 ? std::string{"000"} + std::to_string(i) : std::string{"00"} + std::to_string(i);
        this->storage->create(user_0, "key_" + suffix, "value_" + suffix);
    }

    for (auto i : boost::irange(1, 10))
    {
        size_t count{};
        auto res = this->storage->read_if(user_0, "", "",
            [&](const std::string& /*key*/, const std::string& /*value*/) -> bool
            {
                return ++count % i == 0;
            });

        EXPECT_EQ(res.size(), count / i);
    }

    for (auto i : boost::irange(1, 10))
    {
        size_t count{};
        auto res = this->storage->get_keys_if(user_0, "", "",
            [&](const std::string& /*key*/, const std::string& /*value*/) -> bool
            {
                return ++count % i == 0;
            });

        EXPECT_EQ(res.size(), count / i);
    }
}

TYPED_TEST(storageTest, test_regex_match_queries)
{
    const bzn::uuid_t user_0{"b9dc2595-15ee-435a-8af7-7cafc132f527"};

    this->storage->create(user_0, "0001_somehash_1_0", "value");
    this->storage->create(user_0, "0001_somehash_1_1", "value");
    this->storage->create(user_0, "0001_somehash_1_2", "value");

    this->storage->create(user_0, "0002_somehash_1_0", "value");
    this->storage->create(user_0, "0002_somehash_1_1", "value");

    this->storage->create(user_0, "0003_otherhash_1_0", "value");
    this->storage->create(user_0, "0003_somehash_1_11", "value");
    this->storage->create(user_0, "0003_somehash_1_2", "value");

    this->storage->create(user_0, "0004_somehash_2_0", "value");

    this->storage->create(user_0, "0005_otherhash_2_0", "value");
    this->storage->create(user_0, "0005_another_hash_2_1", "value");
    this->storage->create(user_0, "0005_somehash_2_2", "value");

    this->storage->create(user_0, "0006_somehash_1_0", "value");
    this->storage->create(user_0, "0006_somehash_1_1", "value");
    this->storage->create(user_0, "0006_another_hash_1_2", "value");

    EXPECT_EQ(this->storage->read_if(user_0, "", "").size(), 15u);
    EXPECT_EQ(this->storage->read_if(user_0, "0003", "0004").size(), 3u);
    EXPECT_EQ(this->storage->read_if(user_0, "0003", "").size(), 10u);
    EXPECT_EQ(this->storage->read_if(user_0, "0003", "0003").size(), 0u);
    EXPECT_EQ(this->storage->read_if(user_0, "0003", "0002").size(), 0u);

    std::regex exp1(".*_.*_1_.*");
    auto match_key1 = [&](const std::string& key, const std::string& /*value*/)->bool
    {
        return std::regex_search(key, exp1, std::regex_constants::match_continuous);
    };

    EXPECT_EQ(this->storage->read_if(user_0, "", "", match_key1).size(), 11u);
    EXPECT_EQ(this->storage->read_if(user_0, "0002", "", match_key1).size(), 8u);
    EXPECT_EQ(this->storage->read_if(user_0, "0002", "0006", match_key1).size(), 5u);

    std::regex exp2(".*_.*_.*_1$");
    auto match_key2 = [&](const std::string& key, const std::string& /*value*/)->bool
    {
        return std::regex_search(key, exp2, std::regex_constants::match_continuous);
    };

    EXPECT_EQ(this->storage->read_if(user_0, "", "", match_key2).size(), 4u);
    EXPECT_EQ(this->storage->read_if(user_0, "0002", "", match_key2).size(), 3u);
    EXPECT_EQ(this->storage->read_if(user_0, "0002", "0006", match_key2).size(), 2u);

    std::regex exp3(".*_.*_.*_[1-2]$");
    auto match_key3 = [&](const std::string& key, const std::string& /*value*/)->bool
    {
        return std::regex_search(key, exp3, std::regex_constants::match_continuous);
    };

    EXPECT_EQ(this->storage->read_if(user_0, "", "", match_key3).size(), 8u);
    EXPECT_EQ(this->storage->read_if(user_0, "0002", "", match_key3).size(), 6u);
    EXPECT_EQ(this->storage->read_if(user_0, "0002", "0006", match_key3).size(), 4u);
}
