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

#include <gtest/gtest.h>
#include <include/bluzelle.hpp>
#include <pbft/operations/pbft_memory_operation.hpp>
#include <proto/bluzelle.pb.h>
#include <peers_beacon/peer_address.hpp>
#include <pbft/operations/pbft_persistent_operation.hpp>
#include <storage/mem_storage.hpp>
#include <mocks/smart_mock_peers_beacon.hpp>

using namespace ::testing;

namespace
{

    const bzn::uuid_t TEST_NODE_UUID{"uuid4"};

    const std::vector<bzn::peer_address_t> TEST_PEER_LIST{{  "127.0.0.1", 8081, "name1", "uuid1"}
                                           , {"127.0.0.1", 8082, "name2", "uuid2"}
                                           , {"127.0.0.1", 8083, "name3", "uuid3"}
                                           , {"127.0.0.1", 8084, "name4", TEST_NODE_UUID}};

    const std::vector<bzn::peer_address_t> TEST_2F_PEER_LIST{{  "127.0.0.1", 8081, "name1", "uuid1"}
                                              , {"127.0.0.1", 8084, "name4", TEST_NODE_UUID}};

    const std::vector<bzn::peer_address_t> TEST_2F_PLUS_1_PEER_LIST{{  "127.0.0.1", 8081, "name1", "uuid1"}
                                                     , {"127.0.0.1", 8082, "name2", "uuid2"}
                                                     , {"127.0.0.1", 8084, "name4", TEST_NODE_UUID}};


    class pbft_operation_test_common : public Test
    {
    public:
        bzn_envelope request;
        bzn::hash_t request_hash = "somehash";
        uint64_t view = 6;
        uint64_t sequence = 19;

        std::shared_ptr<bzn::storage_base> storage = std::make_shared<bzn::mem_storage>();

        std::vector<std::shared_ptr<bzn::pbft_operation>> operations{
            std::make_shared<bzn::pbft_memory_operation>(view, sequence, request_hash, static_peers_beacon_for(TEST_PEER_LIST)),
            std::make_shared<bzn::pbft_persistent_operation>(view, sequence, request_hash, storage, static_peers_beacon_for(TEST_PEER_LIST))
        };

        bzn_envelope empty_original_msg;

        pbft_msg preprepare;
        pbft_msg prepare;
        pbft_msg commit;

        pbft_operation_test_common()
        {
            database_msg msg;
            request.set_database_msg(msg.SerializeAsString());

            this->preprepare.set_type(pbft_msg_type::PBFT_MSG_PREPREPARE);
            this->prepare.set_type(pbft_msg_type::PBFT_MSG_PREPARE);
            this->commit.set_type(pbft_msg_type::PBFT_MSG_COMMIT);
        }
    };


    TEST_F(pbft_operation_test_common, initially_unprepared)
    {
        for (const auto& op : this->operations){
            EXPECT_FALSE(op->is_prepared());
        }
    }


    TEST_F(pbft_operation_test_common, prepared_after_all_msgs)
    {
        for (const auto& op : this->operations)
        {
            op->record_pbft_msg(this->preprepare, this->empty_original_msg);
            op->record_request(this->request);

            for (const auto& peer : TEST_PEER_LIST)
            {
                bzn_envelope msg;
                msg.set_sender(peer.uuid);
                op->record_pbft_msg(this->prepare, msg);
            }

            EXPECT_TRUE(op->is_prepared());
        }
    }

    TEST_F(pbft_operation_test_common, not_prepared_without_request)
    {
        for (const auto& op : this->operations)
        {
            op->record_pbft_msg(this->preprepare, this->empty_original_msg);

            for (const auto& peer : TEST_PEER_LIST)
            {
                bzn_envelope msg;
                msg.set_sender(peer.uuid);
                op->record_pbft_msg(this->prepare, msg);
            }

            EXPECT_FALSE(op->is_prepared());
        }
    }


    TEST_F(pbft_operation_test_common, not_prepared_without_preprepare)
    {
        for (const auto& op : this->operations)
        {
            for (const auto& peer : TEST_PEER_LIST)
            {
                bzn_envelope msg;
                msg.set_sender(peer.uuid);
                op->record_pbft_msg(this->prepare, msg);
            }

            EXPECT_FALSE(op->is_prepared());
        }
    }


    TEST_F(pbft_operation_test_common, not_prepared_with_2f)
    {
        for (const auto& op : this->operations)
        {
            op->record_pbft_msg(this->preprepare, this->empty_original_msg);

            for (const auto& peer : TEST_2F_PEER_LIST)
            {
                bzn_envelope msg;
                msg.set_sender(peer.uuid);
                op->record_pbft_msg(this->prepare, msg);
            }

            EXPECT_FALSE(op->is_prepared());
        }
    }


    TEST_F(pbft_operation_test_common, prepared_with_2f_PLUS_1)
    {
        for (const auto& op : this->operations)
        {
            op->record_pbft_msg(this->preprepare, this->empty_original_msg);
            op->record_request(this->request);

            for (const auto& peer : TEST_2F_PLUS_1_PEER_LIST)
            {
                bzn_envelope msg;
                msg.set_sender(peer.uuid);
                op->record_pbft_msg(this->prepare, msg);
            }

            EXPECT_TRUE(op->is_prepared());
        }
    }

    TEST_F(pbft_operation_test_common, remembers_preprepare)
    {
        for (const auto& op : this->operations)
        {
            bzn_envelope env;
            env.set_sender("alice");
            op->record_pbft_msg(this->preprepare, env);

            EXPECT_EQ(op->get_preprepare().sender(), "alice");
        }
    }

    TEST_F(pbft_operation_test_common, remembers_multiple_distinct_prepares)
    {
        for (const auto& op : this->operations)
        {
            bzn_envelope env;

            env.set_sender("alice");
            op->record_pbft_msg(this->prepare, env);

            env.set_sender("bob");
            op->record_pbft_msg(this->prepare, env);

            env.set_sender("carlos");
            op->record_pbft_msg(this->prepare, env);

            EXPECT_EQ(op->get_prepares().size(), 3u);
        }
    }

    TEST_F(pbft_operation_test_common, ignores_duplicate_prepares)
    {
        for (const auto& op : this->operations)
        {
            bzn_envelope env;

            op->record_request(this->request);
            op->record_pbft_msg(this->preprepare, env);

            env.set_sender("alice");
            op->record_pbft_msg(this->prepare, env);

            env.set_sender("mallory");
            op->record_pbft_msg(this->prepare, env);
            op->record_pbft_msg(this->prepare, env);
            op->record_pbft_msg(this->prepare, env);

            EXPECT_EQ(op->get_prepares().size(), 2u);
            EXPECT_FALSE(op->is_prepared());
        }
    }
}
