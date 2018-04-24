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

using namespace ::testing;


class storageTest : public Test
{
public:
    storageTest()
    {
        this->mock_node_base = std::make_shared<bzn::Mocknode_base>();

        // intercept a registration to "crud" messages...
        EXPECT_CALL(*this->mock_node_base, register_for_message("crud", _)).WillOnce(Invoke(
            [&](const auto& /*msg*/, auto handler)
            {
                this->msg_handler = handler;
                return true;
            }));

        this->storage = std::make_shared<bzn::storage>(mock_node_base);

        storage->start();
    }


    bzn::message_handler msg_handler;
    std::shared_ptr<bzn::Mocknode_base> mock_node_base;
    std::shared_ptr<bzn::storage_base> storage;
};


TEST_F(storageTest, test_create_and_read)
{
//    bzn::message msg;
//
//    msg["bzn-api"] = "crud";
//    msg["cmd"] = "create";
//    msg["data"]["key"] = "bluzelle.json";
//    msg["data"]["value"] = "ewogICJsaXN0ZW5lcl9hZGRyZXNzIiA6ICIxMjcuMC4wLjEiLAogICJsaXN0ZW5lcl9wb3J0IiA6"
//        "IDQ5MTUyLAogICJldGhlcmV1bSIgOiAiMHgwMDZlYWU3MjA3NzQ0OWNhY2E5MTA3OGVmNzg1NTJj"
//        "MGNkOWJjZThmIgp9Cg==";
//
//    this->storage->create("", msg["key"], msg["value"]);
//
}


TEST(storage, test_read)
{

}


TEST(storage, test_update)
{

}


TEST(storage, test_delete)
{

}
