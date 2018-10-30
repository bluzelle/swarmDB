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
#include <pbft/test/pbft_proto_test.hpp>
#include <utils/make_endpoint.hpp>

using namespace ::testing;

namespace bzn
{
    namespace test
    {
        pbft_membership_msg
        extract_pbft_membership_msg(const std::string& msg)
        {
            bzn_envelope outer;
            outer.ParseFromString(msg);
            pbft_membership_msg result;
            result.ParseFromString(outer.pbft_membership());
            return result;
        }

        bool
        is_get_state(std::shared_ptr<std::string> wrapped_msg)
        {
            pbft_membership_msg msg = extract_pbft_membership_msg(*wrapped_msg);

            return msg.type() == PBFT_MMSG_GET_STATE && msg.sequence() > 0 && !(extract_sender(*wrapped_msg).empty())
                && !msg.state_hash().empty();
        }

        bool
        is_set_state(std::shared_ptr<std::string> wrapped_msg)
        {
            pbft_membership_msg msg = extract_pbft_membership_msg(*wrapped_msg);

            return msg.type() == PBFT_MMSG_SET_STATE && msg.sequence() > 0 && !(extract_sender(*wrapped_msg).empty())
                   && msg.state_hash() != "";
        }
    }

    using namespace test;

    class pbft_catchup_test : public pbft_proto_test
    {
    public:

        void send_get_state_request(uint64_t sequence)
        {
            pbft_membership_msg msg;
            msg.set_type(PBFT_MMSG_GET_STATE);
            msg.set_sequence(sequence);
            msg.set_state_hash(std::to_string(sequence));
            auto wmsg = wrap_pbft_membership_msg(msg);

            this->membership_handler(wmsg, this->mock_session);
        }
    };

    TEST_F(pbft_catchup_test, node_requests_state_after_unknown_checkpoint)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        // node shouldn't be sending any checkpoint messages right now
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_checkpoint, Eq(true))))
            .Times((Exactly(0)));

        auto nodes = TEST_PEER_LIST.begin();
        size_t req_nodes = 2 * this->faulty_nodes_bound();
        for (size_t i = 0; i < req_nodes; i++)
        {
            bzn::peer_address_t node(*nodes++);
            send_checkpoint(node, 100);
        }

        // one more checkpoint message and the node should request state from primary
        auto primary = this->pbft->get_primary();
        EXPECT_CALL(*mock_node, send_message_str(make_endpoint(primary), ResultOf(is_get_state, Eq(true))))
            .Times((Exactly(1)));

        bzn::peer_address_t node(*nodes++);
        send_checkpoint(node, 100);
    }

    TEST_F(pbft_catchup_test, node_doesnt_request_state_after_known_checkpoint)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        prepare_for_checkpoint(100);
        for (size_t i = 0; i < 100; i++)
        {
            run_transaction_through_backup();
        }

        // since the node has this checkpoint it should NOT request state for it
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_get_state, Eq(true))))
            .Times((Exactly(0)));
        stabilize_checkpoint(100);
    }

    TEST_F(pbft_catchup_test, primary_provides_state)
    {
        this->build_pbft();

        for (size_t i = 0; i < 99; i++)
        {
            run_transaction_through_primary();
        }
        prepare_for_checkpoint(100);
        run_transaction_through_primary();
        stabilize_checkpoint(100);

        EXPECT_CALL(*mock_session, send_datagram(ResultOf(is_set_state, Eq(true))))
            .Times((Exactly(1)));
        send_get_state_request(100);
    }

    TEST_F(pbft_catchup_test, node_adopts_requested_checkpoint)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        // get the node to request state from primary
        auto primary = this->pbft->get_primary();
        EXPECT_CALL(*mock_node, send_message_str(make_endpoint(primary), ResultOf(is_get_state, Eq(true))))
            .Times((Exactly(1)));

        auto nodes = TEST_PEER_LIST.begin();
        size_t req_nodes = 2 * this->faulty_nodes_bound() + 1;
        for (size_t i = 0; i < req_nodes; i++)
        {
            bzn::peer_address_t node(*nodes++);
            send_checkpoint(node, 100);
        }

        // send the node the checkpoint "data"
        pbft_membership_msg reply;
        reply.set_type(PBFT_MMSG_SET_STATE);
        reply.set_sequence(100);
        reply.set_state_hash("100");
        reply.set_state_data("state_100");
        auto wmsg = wrap_pbft_membership_msg(reply);
        this->membership_handler(wmsg, nullptr);

        EXPECT_EQ(this->pbft->latest_stable_checkpoint(), checkpoint_t(100, "100"));
    }

    TEST_F(pbft_catchup_test, node_doesnt_adopt_wrong_checkpoint)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        // get the node to request state from primary
        auto primary = this->pbft->get_primary();
        EXPECT_CALL(*mock_node, send_message_str(make_endpoint(primary), ResultOf(is_get_state, Eq(true))))
            .Times((Exactly(1)));

        auto nodes = TEST_PEER_LIST.begin();
        size_t req_nodes = 2 * this->faulty_nodes_bound() + 1;
        for (size_t i = 0; i < req_nodes; i++)
        {
            bzn::peer_address_t node(*nodes++);
            send_checkpoint(node, 100);
        }

        // send the node the checkpoint "data"
        pbft_membership_msg reply;
        reply.set_type(PBFT_MMSG_SET_STATE);
        reply.set_sequence(200);
        reply.set_state_hash("200");
        reply.set_state_data("state_200");
        auto wmsg = wrap_pbft_membership_msg(reply);
        this->membership_handler(wmsg, nullptr);

        EXPECT_NE(this->pbft->latest_stable_checkpoint(), checkpoint_t(200, "200"));
    }
}