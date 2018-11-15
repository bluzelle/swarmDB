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
#include <set>
#include <mocks/mock_session_base.hpp>
#include <utils/make_endpoint.hpp>
#include <gtest/gtest.h>

namespace bzn::test
{

    TEST_F(pbft_test, test_requests_create_operations)
    {
        this->build_pbft();
        ASSERT_TRUE(pbft->is_primary());
        ASSERT_EQ(0u, pbft->outstanding_operations_count());
        pbft->handle_database_message(this->request_msg, this->mock_session);
        ASSERT_EQ(1u, pbft->outstanding_operations_count());
    }

    TEST_F(pbft_test, test_requests_fire_preprepare)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_preprepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        pbft->handle_database_message(this->request_msg, this->mock_session);
    }

    TEST_F(pbft_test, test_forwarded_to_primary_when_not_primary)
    {
        EXPECT_CALL(*mock_node, send_message(_, A<std::shared_ptr<bzn_envelope>>())).Times(1).WillRepeatedly(Invoke(
                [&](auto ep, auto msg)
                {
                    EXPECT_EQ(ep, make_endpoint(this->pbft->get_primary()));
                    EXPECT_EQ(msg->payload_case(), bzn_envelope::kDatabaseMsg);
                }));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();
        EXPECT_FALSE(pbft->is_primary());

        pbft->handle_database_message(this->request_msg, this->mock_session);

    }

    std::set<uint64_t> seen_sequences;

    void
    save_sequences(const boost::asio::ip::tcp::endpoint& /*ep*/, std::shared_ptr<bzn_envelope> wrapped_msg)
    {
        pbft_msg msg;
        msg.ParseFromString(wrapped_msg->pbft());
        seen_sequences.insert(msg.sequence());
    }


    TEST_F(pbft_test, test_different_requests_get_different_sequences)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message(_, _)).WillRepeatedly(Invoke(save_sequences));

        database_msg req, req2;
        req.mutable_header()->set_transaction_id(5);
        req2.mutable_header()->set_transaction_id(1055);

        seen_sequences = std::set<uint64_t>();
        pbft->handle_database_message(wrap_request(req), this->mock_session);
        pbft->handle_database_message(wrap_request(req2), this->mock_session);
        ASSERT_EQ(seen_sequences.size(), 2u);
    }

    TEST_F(pbft_test, test_preprepare_triggers_prepare)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->pbft->handle_message(this->preprepare_msg, default_original_msg);
    }

    TEST_F(pbft_test, test_prepare_contains_uuid)
    {
        this->build_pbft();
        std::shared_ptr<bzn_envelope> wrapped_msg;
        EXPECT_CALL(*mock_node, send_message(_, _)).WillRepeatedly(SaveArg<1>(&wrapped_msg));

        this->pbft->handle_message(this->preprepare_msg, default_original_msg);

        pbft_msg msg_sent;
        msg_sent.ParseFromString(wrapped_msg->pbft());

        ASSERT_EQ(wrapped_msg->sender(), this->pbft->get_uuid());
        ASSERT_EQ(wrapped_msg->sender(), TEST_NODE_UUID);
    }

    TEST_F(pbft_test, test_wrong_view_preprepare_rejected)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message(_, _)).Times(Exactly(0));

        pbft_msg preprepare2(this->preprepare_msg);
        preprepare2.set_view(6);

        this->pbft->handle_message(preprepare2, default_original_msg);
    }

    TEST_F(pbft_test, test_no_duplicate_prepares_same_sequence_number)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message(_, _)).Times(Exactly(TEST_PEER_LIST.size()));

        pbft_msg preprepare2(this->preprepare_msg);
        preprepare2.set_request_hash("some other hash");

        this->pbft->handle_message(this->preprepare_msg, default_original_msg);
        this->pbft->handle_message(preprepare2, default_original_msg);
    }

    TEST_F(pbft_test, test_commit_messages_sent)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_commit, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->pbft->handle_message(this->preprepare_msg, default_original_msg);
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg prepare = pbft_msg(this->preprepare_msg);
            prepare.set_type(PBFT_MSG_PREPARE);
            this->pbft->handle_message(prepare, from(peer.uuid));
        }
    }

    TEST_F(pbft_test, test_commits_applied)
    {
        EXPECT_CALL(*(this->mock_io_context), post(_)).Times(Exactly(1));
        this->build_pbft();

        pbft_msg preprepare = pbft_msg(this->preprepare_msg);
        preprepare.set_sequence(1);
        this->pbft->handle_message(preprepare, default_original_msg);

        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg prepare = pbft_msg(preprepare);
            pbft_msg commit = pbft_msg(preprepare);
            prepare.set_type(PBFT_MSG_PREPARE);
            commit.set_type(PBFT_MSG_COMMIT);
            this->pbft->handle_message(prepare, from(peer.uuid));
            this->pbft->handle_message(commit, from(peer.uuid));
        }
    }

    TEST_F(pbft_test, dummy_pbft_service_does_not_crash)
    {
        database_msg db;
        mock_service->query(db, 0);
        mock_service->consolidate_log(2);
    }

    TEST_F(pbft_test, client_request_results_in_message_ack)
    {
        this->build_pbft();
        auto mock_session = std::make_shared<NiceMock<bzn::Mocksession_base>>();

        EXPECT_CALL(*mock_session, send_message(A<std::shared_ptr<std::string>>(), _)).Times(Exactly(1));

        this->database_handler(this->request_msg, mock_session);
    }

    TEST_F(pbft_test, client_request_executed_results_in_message_response)
    {
        auto mock_session = std::make_shared<bzn::Mocksession_base>();
        EXPECT_CALL(*mock_session, send_datagram(_)).Times(Exactly(1));

        auto peers = std::make_shared<const std::vector<bzn::peer_address_t>>();
        auto op = std::make_shared<pbft_operation>(1, 1, "somehash", peers);
        op->set_session(mock_session);

        dummy_pbft_service service(this->mock_io_context);
        service.register_execute_handler([](auto){});

        service.apply_operation(op);
    }

    TEST_F(pbft_test, messages_after_high_water_mark_dropped)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(0));

        this->preprepare_msg.set_sequence(pbft->get_high_water_mark() + 1);
        pbft->handle_message(preprepare_msg, default_original_msg);
    }

    TEST_F(pbft_test, messages_before_low_water_mark_dropped)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(0));

        this->preprepare_msg.set_sequence(this->pbft->get_low_water_mark());
        pbft->handle_message(preprepare_msg, default_original_msg);
    }

    TEST_F(pbft_test, request_redirect_to_primary_notifies_failure_detector) {
        EXPECT_CALL(*mock_failure_detector, request_seen(_)).Times(Exactly(1));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        EXPECT_FALSE(pbft->is_primary());
        pbft->handle_database_message(this->request_msg, this->mock_session);
    }

    MATCHER(operation_ptr_has_session, "")
    {
        return arg->has_session();
    }

    TEST_F(pbft_test, request_redirect_attaches_session) {

        EXPECT_CALL(*mock_service, apply_operation(operation_ptr_has_session()));
        EXPECT_CALL(*mock_io_context, post(_)).WillRepeatedly(Invoke(
                [](const auto task)
                {
                    task();
                }
                ));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        EXPECT_FALSE(pbft->is_primary());
        this->request_msg.set_timestamp(now());
        pbft->handle_database_message(this->request_msg, this->mock_session);

        auto hash = this->crypto->hash(this->request_msg);

        this->send_preprepare(1, 1, hash, this->request_msg);
        this->send_prepares(1, 1, hash);
        this->send_commits(1, 1, hash);
    }

    TEST_F(pbft_test, late_request_redirect_attaches_session) {

        EXPECT_CALL(*mock_service, apply_operation(operation_ptr_has_session()));
        EXPECT_CALL(*mock_io_context, post(_)).WillRepeatedly(Invoke(
                [](const auto task)
                {
                    task();
                }
        ));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        EXPECT_FALSE(pbft->is_primary());
        this->request_msg.set_timestamp(now());
        auto hash = this->crypto->hash(this->request_msg);

        this->send_preprepare(1, 1, hash, this->request_msg);
        this->send_prepares(1, 1, hash);

        pbft->handle_database_message(this->request_msg, this->mock_session);

        this->send_commits(1, 1, hash);
    }
}
