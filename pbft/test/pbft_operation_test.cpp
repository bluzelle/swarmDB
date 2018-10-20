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
#include <pbft/pbft_operation.hpp>
#include <proto/bluzelle.pb.h>

using namespace ::testing;

namespace
{

    const bzn::uuid_t TEST_NODE_UUID{"uuid4"};

    const std::vector<bzn::peer_address_t> TEST_PEER_LIST{{  "127.0.0.1", 8081, 8881, "name1", "uuid1"}
                                           , {"127.0.0.1", 8082, 8882, "name2", "uuid2"}
                                           , {"127.0.0.1", 8083, 8883, "name3", "uuid3"}
                                           , {"127.0.0.1", 8084, 8884, "name4", TEST_NODE_UUID}};

    const std::vector<bzn::peer_address_t> TEST_2F_PEER_LIST{{  "127.0.0.1", 8081, 8881, "name1", "uuid1"}
                                              , {"127.0.0.1", 8084, 8884, "name4", TEST_NODE_UUID}};

    const std::vector<bzn::peer_address_t> TEST_2F_PLUS_1_PEER_LIST{{  "127.0.0.1", 8081, 8881, "name1", "uuid1"}
                                                     , {"127.0.0.1", 8082, 8882, "name2", "uuid2"}
                                                     , {"127.0.0.1", 8084, 8884, "name4", TEST_NODE_UUID}};


    class pbft_operation_test : public Test
    {
    public:
        pbft_request request;
        uint64_t view = 6;
        uint64_t sequence = 19;

        bzn::pbft_operation op;

        wrapped_bzn_msg empty_original_msg;

        pbft_operation_test()
                : op(view, sequence, request, std::make_shared<std::vector<bzn::peer_address_t>>(TEST_PEER_LIST))
        {
        }
    };


    TEST_F(pbft_operation_test, initially_unprepared)
    {
        EXPECT_FALSE(this->op.is_prepared());
    }


    TEST_F(pbft_operation_test, prepared_after_all_msgs)
    {
        wrapped_bzn_msg preprepare;
        this->op.record_preprepare(preprepare);

        for (const auto& peer : TEST_PEER_LIST)
        {
            wrapped_bzn_msg msg;
            msg.set_sender(peer.uuid);
            op.record_prepare(msg);
        }

        EXPECT_TRUE(this->op.is_prepared());
    }


    TEST_F(pbft_operation_test, not_prepared_without_preprepare)
    {
        for (const auto& peer : TEST_PEER_LIST)
        {
            wrapped_bzn_msg msg;
            msg.set_sender(peer.uuid);
            op.record_prepare(msg);
        }

        EXPECT_FALSE(this->op.is_prepared());
    }


    TEST_F(pbft_operation_test, not_prepared_with_2f)
    {
        wrapped_bzn_msg preprepare;
        this->op.record_preprepare(preprepare);

        for (const auto& peer : TEST_2F_PEER_LIST)
        {
            wrapped_bzn_msg msg;
            msg.set_sender(peer.uuid);
            op.record_prepare(msg);
        }

        EXPECT_FALSE(this->op.is_prepared());
    }


    TEST_F(pbft_operation_test, prepared_with_2f_PLUS_1)
    {
        wrapped_bzn_msg preprepare;
        this->op.record_preprepare(preprepare);

        for (const auto& peer : TEST_2F_PLUS_1_PEER_LIST)
        {
            wrapped_bzn_msg msg;
            msg.set_sender(peer.uuid);
            op.record_prepare(msg);
        }

        EXPECT_TRUE(this->op.is_prepared());
    }
}
