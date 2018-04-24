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

#include <storage/storage.hpp>
#include <mocks/mock_node_base.hpp>
#include <storage/storage_base.hpp>
#include <boost/chrono.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

using namespace ::testing;

namespace
{
    const bzn::uuid_t user_uuid = "4bba2aeb-44fe-441e-bb6b-8817561eb716";
    const std::string key = "bluzelle.json";
    const std::string value = "ewogICJsaXN0ZW5lcl9hZGRyZXNzIiA6ICIxMjcuMC4wLjEiLAogICJsaXN0ZW5lcl9wb3J0IiA6"
                              "IDQ5MTUyLAogICJldGhlcmV1bSIgOiAiMHgwMDZlYWU3MjA3NzQ0OWNhY2E5MTA3OGVmNzg1NTJj"
                              "MGNkOWJjZThmIgp9Cg==";
    const std::string path{"storage_test.dat"};

    boost::random::mt19937 gen;

    std::string
    generate_test_string(int n = 100)
    {
        std::string test_data;
        test_data.resize(n);

        boost::random::uniform_int_distribution<> dist('a', 'z');
        for(auto& c : test_data)
        {
            c = (char)dist(gen);
        }
        return test_data;
    }


    std::vector<std::string>
    create_test_records_in_storage(std::shared_ptr<bzn::storage_base> storage, int n_elements = 100)
    {
        std::vector<std::string> keys;
        const bzn::uuid_t user_uuid = "4bba2aeb-44fe-441e-bb6b-8817561eb716";
        const boost::random::uniform_int_distribution<> dist(10, 4000);

        for(int i=0;i<n_elements; ++i)
        {
            std::string key{"key."};
            key.append(std::to_string(i));
            storage->create(user_uuid, key, generate_test_string(dist(gen)));
            keys.emplace_back(key);
        }
        return keys;
    }

}


class storageTest : public Test
{
public:
    storageTest()
    {
        this->storage = std::make_shared<bzn::storage>();
    }
    std::shared_ptr<bzn::storage_base> storage;
};


TEST_F(storageTest, test_create_and_read)
{
    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(user_uuid, key, value));

    const auto returned_record = this->storage->read(user_uuid, key);

    EXPECT_TRUE(returned_record->transaction_id.size()>0); // todo: maybe check for valid uuid?

    const auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());

    EXPECT_TRUE( (now.count() - returned_record->timestamp.count()) >= 0 );

    EXPECT_EQ(returned_record->value, value);
}


TEST_F(storageTest, test_create_existing)
{
    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(user_uuid, key, value));

    EXPECT_EQ(bzn::storage_base::result::exists, this->storage->create(user_uuid, key, value));
}


TEST_F(storageTest, test_read_null)
{
    EXPECT_EQ(nullptr, this->storage->read(user_uuid, "nokey"));
}


TEST_F(storageTest, test_update)
{
    const std::string updated_value = "I have changed the value of the text";

    // try updating a record that does not exist
    EXPECT_EQ(bzn::storage_base::result::not_found, this->storage->update(user_uuid, key, updated_value));

    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(user_uuid, key, value));

    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->update(user_uuid, key, updated_value));

    const auto returned_record = this->storage->read(user_uuid, key);

    EXPECT_EQ(returned_record->value, updated_value);
}


TEST_F(storageTest, test_delete)
{
    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->create(user_uuid, key, value));

    const auto  returned_record = this->storage->read(user_uuid, key);

    EXPECT_EQ(returned_record->value, value);

    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->remove(user_uuid, key));

    EXPECT_EQ(this->storage->read(user_uuid, key), nullptr);

    EXPECT_EQ(bzn::storage_base::result::not_found, this->storage->remove(user_uuid, key));
}


TEST_F(storageTest, test_save)
{
    boost::filesystem::remove(path);

    EXPECT_FALSE(boost::filesystem::exists(path));

    create_test_records_in_storage(this->storage);

    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->save(path));

    EXPECT_TRUE(boost::filesystem::exists(path));

    boost::filesystem::remove(path);
}


TEST_F(storageTest, test_save_load)
{
    boost::filesystem::remove(path);

    EXPECT_FALSE(boost::filesystem::exists(path));

    const auto keys{create_test_records_in_storage(this->storage, 400)};

    EXPECT_EQ(bzn::storage_base::result::ok, this->storage->save(path));

    EXPECT_TRUE(boost::filesystem::exists(path));

    const auto storage_copy = std::make_shared<bzn::storage>();

    EXPECT_EQ(bzn::storage_base::result::ok, storage_copy->load(path));

    for(const auto& key : keys)
    {
        const auto record = this->storage->read(user_uuid, key);
        const auto record_copy = storage_copy->read(user_uuid, key);

        EXPECT_EQ(record->timestamp, record_copy->timestamp);
        EXPECT_EQ(record->transaction_id, record_copy->transaction_id);
        EXPECT_EQ(record->value, record_copy->value);
    }

    boost::filesystem::remove(path);
}


TEST_F(storageTest, test_file_load_fail)
{
    std::string bad_path{"not_existing.dat"};
    EXPECT_EQ(bzn::storage_base::result::not_found, this->storage->load(bad_path));
}


TEST_F(storageTest, test_file_save_fail)
{
    std::string bad_path{"/not_existing.dat"};
    EXPECT_EQ(bzn::storage_base::result::not_saved, this->storage->save(bad_path));
}


TEST_F(storageTest, test_get_keys_returns_all_keys)
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

    if(user_0_keys.size() >0)
    {
        EXPECT_TRUE(std::equal(user_keys.begin(), user_keys.end(), user_0_keys.begin() ));
        EXPECT_TRUE(std::equal(user_keys.begin(), user_keys.end(), user_1_keys.begin() ));
        EXPECT_TRUE(std::equal(user_keys.begin(), user_keys.end(), user_2_keys.begin() ));
    }
}

