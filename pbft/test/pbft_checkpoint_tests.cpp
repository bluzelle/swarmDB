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
#include <boost/range/irange.hpp>
#include <pbft/pbft_memory_operation.hpp>

namespace bzn::test
{

    const bzn::hash_t cp0_hash = "db state hash initial_state";
    const bzn::hash_t cp1_hash = "db state hash cp 1";
    const bzn::hash_t cp2_hash = "db state hash cp 2";

    pbft_msg cp1_msg;

    class pbft_checkpoint_test : public pbft_test
    {
    public:
        pbft_checkpoint_test()
        {
            EXPECT_CALL(*mock_service, service_state_hash(0))
                .Times(AnyNumber()).WillRepeatedly(Return(cp0_hash));

            EXPECT_CALL(*mock_service, service_state_hash(CHECKPOINT_INTERVAL))
                    .Times(AnyNumber()).WillRepeatedly(Return(cp1_hash));

            EXPECT_CALL(*mock_service, service_state_hash(CHECKPOINT_INTERVAL*2))
                    .Times(AnyNumber()).WillRepeatedly(Return(cp2_hash));

            cp1_msg.set_type(PBFT_MSG_CHECKPOINT);
            cp1_msg.set_sequence(CHECKPOINT_INTERVAL);
            cp1_msg.set_state_hash(cp1_hash);
        }

    };

    TEST_F(pbft_checkpoint_test, test_checkpoint_messages_sent_on_execute)
    {
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_checkpoint, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->build_pbft();
        auto op = std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL, "somehash", nullptr);
        this->service_execute_handler(op);
    }

    TEST_F(pbft_checkpoint_test, no_checkpoint_on_message_before_local_state)
    {
        this->build_pbft();
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg msg = cp1_msg;
            this->pbft->handle_message(msg, from(peer.uuid));
        }

        EXPECT_EQ(0u, this->pbft->latest_checkpoint().first);
        EXPECT_EQ(0u, this->pbft->unstable_checkpoints_count());
    }

    TEST_F(pbft_checkpoint_test, unstable_checkpoint_on_local_state_before_message)
    {
        this->build_pbft();
        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL, "somehash", nullptr));

        EXPECT_EQ(CHECKPOINT_INTERVAL, this->pbft->latest_checkpoint().first);
        EXPECT_EQ(0u, this->pbft->latest_stable_checkpoint().first);
        EXPECT_EQ(1u, this->pbft->unstable_checkpoints_count());
    }

    TEST_F(pbft_checkpoint_test, stable_checkpoint_on_message_after_local_state)
    {
        this->build_pbft();
        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL, "somehash", nullptr));
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg msg = cp1_msg;
            this->pbft->handle_message(msg, from(peer.uuid));
        }

        EXPECT_EQ(CHECKPOINT_INTERVAL, this->pbft->latest_checkpoint().first);
        EXPECT_EQ(CHECKPOINT_INTERVAL, this->pbft->latest_stable_checkpoint().first);
        EXPECT_EQ(0u, this->pbft->unstable_checkpoints_count());
    }

    TEST_F(pbft_checkpoint_test, stable_checkpoint_on_local_state_after_message)
    {
        this->build_pbft();
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg msg = cp1_msg;
            this->pbft->handle_message(msg, from(peer.uuid));
        }
        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL, "somehash", nullptr));

        EXPECT_EQ(CHECKPOINT_INTERVAL, this->pbft->latest_checkpoint().first);
        EXPECT_EQ(CHECKPOINT_INTERVAL, this->pbft->latest_stable_checkpoint().first);
        EXPECT_EQ(0u, this->pbft->unstable_checkpoints_count());
    }

    TEST_F(pbft_checkpoint_test, unstable_checkpoint_does_not_discard_stable_checkpoint)
    {
        this->build_pbft();
        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL, "somehash", nullptr));
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg msg = cp1_msg;
            this->pbft->handle_message(msg, from(peer.uuid));
        }
        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL*2, "somehash", nullptr));

        EXPECT_EQ(CHECKPOINT_INTERVAL*2, this->pbft->latest_checkpoint().first);
        EXPECT_EQ(CHECKPOINT_INTERVAL, this->pbft->latest_stable_checkpoint().first);
        EXPECT_EQ(1u, this->pbft->unstable_checkpoints_count());
    }

    TEST_F(pbft_checkpoint_test, stable_checkpoint_discards_old_stable_checkpoint)
    {
        this->build_pbft();
        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL, "somehash", nullptr));
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg msg = cp1_msg;
            this->pbft->handle_message(msg, from(peer.uuid));
        }
        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL*2, "somehash", nullptr));
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg msg = cp1_msg;
            msg.set_sequence(CHECKPOINT_INTERVAL*2);
            msg.set_state_hash(cp2_hash);
            this->pbft->handle_message(msg, from(peer.uuid));
        }

        EXPECT_EQ(CHECKPOINT_INTERVAL*2, this->pbft->latest_stable_checkpoint().first);
        EXPECT_EQ(CHECKPOINT_INTERVAL*2, this->pbft->latest_checkpoint().first);
        EXPECT_EQ(0u, this->pbft->unstable_checkpoints_count());
    }

    TEST_F(pbft_checkpoint_test, stable_checkpoint_discards_old_state)
    {
        this->build_pbft();
        for (auto i : boost::irange(1, 10))
        {
            pbft_msg msg;
            msg.set_type(PBFT_MSG_PREPREPARE);
            msg.set_view(1);
            msg.set_sequence(i);
            msg.set_request_hash("somehash");

            this->pbft->handle_message(msg, default_original_msg);
        }

        EXPECT_EQ(9u, this->pbft->outstanding_operations_count());

        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL, "somehash", nullptr));
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg msg = cp1_msg;
            this->pbft->handle_message(msg, from(peer.uuid));
        }

        EXPECT_EQ(0u, this->pbft->outstanding_operations_count());
    }

    TEST_F(pbft_checkpoint_test, initial_checkpoint_matches)
    {
        this->build_pbft();
        auto expected_cp = checkpoint_t(0, INITIAL_CHECKPOINT_HASH);
        EXPECT_EQ(0u, this->pbft->unstable_checkpoints_count());
        EXPECT_EQ(expected_cp, this->pbft->latest_stable_checkpoint());
        EXPECT_EQ(expected_cp, this->pbft->latest_checkpoint());
    }

    TEST_F(pbft_checkpoint_test, stable_checkpoint_advances_high_low_water_marks)
    {
        this->build_pbft();

        uint64_t initial_low = this->pbft->get_low_water_mark();
        uint64_t initial_high = this->pbft->get_high_water_mark();

        this->service_execute_handler(std::make_shared<bzn::pbft_memory_operation>(1, CHECKPOINT_INTERVAL, "somehash", nullptr));

        EXPECT_EQ(this->pbft->get_high_water_mark(), initial_high);
        EXPECT_EQ(this->pbft->get_low_water_mark(), initial_low);

        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg msg = cp1_msg;
            this->pbft->handle_message(msg, from(peer.uuid));
        }

        EXPECT_EQ(CHECKPOINT_INTERVAL, this->pbft->latest_checkpoint().first);
        EXPECT_EQ(CHECKPOINT_INTERVAL, this->pbft->latest_stable_checkpoint().first);
        EXPECT_EQ(0u, this->pbft->unstable_checkpoints_count());

        EXPECT_GT(this->pbft->get_high_water_mark(), initial_high);
        EXPECT_GT(this->pbft->get_low_water_mark(), initial_low);
    }
}
