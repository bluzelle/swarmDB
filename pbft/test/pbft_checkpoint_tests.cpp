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

#include <mocks/smart_mock_node.hpp>
#include <mocks/smart_mock_io.hpp>
#include <mocks/smart_mock_peers_beacon.hpp>
#include <boost/range/irange.hpp>
#include <pbft/operations/pbft_memory_operation.hpp>
#include <storage/mem_storage.hpp>
#include <pbft/pbft_checkpoint_manager.hpp>
#include <pbft/test/pbft_test_common.hpp>
#include <utils/make_endpoint.hpp>

using namespace ::testing;

namespace bzn::test
{
    class pbft_checkpoint_test : public Test
    {
    public:
        std::shared_ptr<bzn::asio::smart_mock_io> mock_io = std::make_shared<bzn::asio::smart_mock_io>();
        std::shared_ptr<bzn::mem_storage> storage = std::make_shared<bzn::mem_storage>();
        std::shared_ptr<bzn::smart_mock_node> node = std::make_shared<bzn::smart_mock_node>();
        std::shared_ptr<bzn::pbft_checkpoint_manager> cp_manager = std::make_shared<bzn::pbft_checkpoint_manager>(mock_io, storage, static_peers_beacon_for(TEST_PEER_LIST), node);

        checkpoint_t cp{100, "100"};
        checkpoint_t cp2{200, "200"};

        void send_checkpoint_messages(const checkpoint_t& cp, size_t count = INT_MAX)
        {
            checkpoint_msg cp_msg;
            cp_msg.set_state_hash(cp.second);
            cp_msg.set_sequence(cp.first);
            for (const auto& peer : TEST_PEER_LIST)
            {
                if (count-- <= 0)
                {
                    break;
                }

                bzn_envelope env;
                env.set_checkpoint_msg(cp_msg.SerializeAsString());
                env.set_sender(peer.uuid);
                this->cp_manager->handle_checkpoint_message(env);
            }
        }

    };

    TEST_F(pbft_checkpoint_test, test_checkpoint_messages_sent_on_execute)
    {
        for (const auto& peer : TEST_PEER_LIST)
        {
            EXPECT_CALL(*node, send_signed_message(make_endpoint(peer), ResultOf(is_checkpoint, Eq(true))));
        }

        this->cp_manager->local_checkpoint_reached(this->cp);
    }

    TEST_F(pbft_checkpoint_test, no_checkpoint_on_message_before_local_state)
    {
        this->send_checkpoint_messages(this->cp);

        EXPECT_EQ(0u, this->cp_manager->get_latest_local_checkpoint().first);
        EXPECT_EQ(100u, this->cp_manager->get_latest_stable_checkpoint().first);
    }

    TEST_F(pbft_checkpoint_test, unstable_checkpoint_on_local_state_before_message)
    {
        for (const auto& peer : TEST_PEER_LIST)
        {
            EXPECT_CALL(*node, send_signed_message(make_endpoint(peer), ResultOf(is_checkpoint, Eq(true))));
        }

        this->cp_manager->local_checkpoint_reached(this->cp);
        EXPECT_EQ(100u, this->cp_manager->get_latest_local_checkpoint().first);
    }

    TEST_F(pbft_checkpoint_test, stable_checkpoint_on_message_after_local_state)
    {
        this->cp_manager->local_checkpoint_reached(this->cp);
        this->send_checkpoint_messages(this->cp);

        EXPECT_EQ(this->cp_manager->get_latest_stable_checkpoint(), this->cp);
        EXPECT_EQ(this->cp_manager->get_latest_local_checkpoint(), this->cp);
    }

    TEST_F(pbft_checkpoint_test, stable_checkpoint_on_local_state_after_message)
    {
        this->send_checkpoint_messages(this->cp);
        this->cp_manager->local_checkpoint_reached(this->cp);

        EXPECT_EQ(this->cp_manager->get_latest_stable_checkpoint(), this->cp);
        EXPECT_EQ(this->cp_manager->get_latest_local_checkpoint(), this->cp);
    }

    TEST_F(pbft_checkpoint_test, unstable_checkpoint_does_not_discard_stable_checkpoint)
    {
        this->send_checkpoint_messages(this->cp);
        this->send_checkpoint_messages(this->cp2, 1); // 4 node swarm; this will not be enough
        this->cp_manager->local_checkpoint_reached(this->cp);
        this->cp_manager->local_checkpoint_reached(this->cp2);
        EXPECT_EQ(this->cp_manager->get_latest_stable_checkpoint(), this->cp);
        EXPECT_EQ(this->cp_manager->get_latest_local_checkpoint(), this->cp2);
        EXPECT_EQ(this->cp_manager->partial_checkpoint_proofs_count(), 1u);
    }

    TEST_F(pbft_checkpoint_test, stable_checkpoint_discards_old_stable_checkpoint)
    {
        this->send_checkpoint_messages(this->cp);
        this->send_checkpoint_messages(this->cp2);
        EXPECT_EQ(this->cp_manager->get_latest_stable_checkpoint(), this->cp2);
        EXPECT_EQ(this->cp_manager->partial_checkpoint_proofs_count(), 0u);
    }

    TEST_F(pbft_checkpoint_test, initial_checkpoint_matches)
    {
        EXPECT_EQ(this->cp_manager->get_latest_stable_checkpoint(), checkpoint_t(0, INITIAL_CHECKPOINT_HASH));
        EXPECT_EQ(this->cp_manager->get_latest_local_checkpoint(), checkpoint_t(0, INITIAL_CHECKPOINT_HASH));
    }
}
