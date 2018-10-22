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
        pbft->handle_database_message(this->request_json, this->mock_session);
        ASSERT_EQ(1u, pbft->outstanding_operations_count());
    }

    TEST_F(pbft_test, test_requests_fire_preprepare)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_preprepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        pbft->handle_database_message(this->request_json, this->mock_session);
    }

    TEST_F(pbft_test, test_forwarded_to_primary_when_not_primary)
    {
        boost::asio::ip::tcp::endpoint msg_sent_to;
        bzn::json_message msg_sent;

        EXPECT_CALL(*mock_node, send_message(_, _)).Times(1).WillRepeatedly(Invoke(
                [&](auto ep, auto msg)
                {
                    EXPECT_EQ(ep, make_endpoint(this->pbft->get_primary()));
                    EXPECT_EQ((*msg)["bzn-api"], "database");
                }));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();
        EXPECT_FALSE(pbft->is_primary());

        pbft->handle_database_message(this->request_json, this->mock_session);

    }

    std::set<uint64_t> seen_sequences;

    void
    save_sequences(const boost::asio::ip::tcp::endpoint& /*ep*/, std::shared_ptr<std::string> wrapped_msg)
    {
        pbft_msg msg = extract_pbft_msg(*wrapped_msg);
        seen_sequences.insert(msg.sequence());
    }


    TEST_F(pbft_test, test_different_requests_get_different_sequences)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message_str(_, _)).WillRepeatedly(Invoke(save_sequences));

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
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->pbft->handle_message(this->preprepare_msg, default_original_msg);
    }

    TEST_F(pbft_test, test_prepare_contains_uuid)
    {
        this->build_pbft();
        std::shared_ptr<std::string> wrapped_msg;
        EXPECT_CALL(*mock_node, send_message_str(_, _)).WillRepeatedly(SaveArg<1>(&wrapped_msg));

        this->pbft->handle_message(this->preprepare_msg, default_original_msg);

        pbft_msg msg_sent = extract_pbft_msg(*wrapped_msg);

        ASSERT_EQ(extract_sender(*wrapped_msg), this->pbft->get_uuid());
        ASSERT_EQ(extract_sender(*wrapped_msg), TEST_NODE_UUID);
    }

    TEST_F(pbft_test, test_wrong_view_preprepare_rejected)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message_str(_, _)).Times(Exactly(0));

        pbft_msg preprepare2(this->preprepare_msg);
        preprepare2.set_view(6);

        this->pbft->handle_message(preprepare2, default_original_msg);
    }

    TEST_F(pbft_test, test_no_duplicate_prepares_same_sequence_number)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message_str(_, _)).Times(Exactly(TEST_PEER_LIST.size()));

        pbft_msg prepreparea(this->preprepare_msg);
        pbft_msg preprepareb(this->preprepare_msg);
        pbft_msg prepreparec(this->preprepare_msg);
        pbft_msg preprepared(this->preprepare_msg);

        preprepareb.mutable_request()->set_timestamp(99);
        prepreparec.mutable_request()->mutable_operation();
        preprepared.mutable_request()->set_client("certainly not bob");

        this->pbft->handle_message(prepreparea, default_original_msg);
        this->pbft->handle_message(preprepareb, default_original_msg);
        this->pbft->handle_message(prepreparec, default_original_msg);
        this->pbft->handle_message(preprepared, default_original_msg);
    }

    TEST_F(pbft_test, test_commit_messages_sent)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_commit, Eq(true))))
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
        mock_service->query(request_msg, 0);
        mock_service->consolidate_log(2);
    }

    TEST_F(pbft_test, client_request_results_in_message_ack)
    {
        auto mock_session = std::make_shared<NiceMock<bzn::Mocksession_base>>();
        std::string last_err;

        EXPECT_CALL(*mock_session, send_message(A<std::shared_ptr<std::string>>(), _)).WillRepeatedly(Invoke(
                [&](auto msg, auto)
                {
                    database_response resp;
                    resp.ParseFromString(*msg);

                    last_err = resp.error().message();
                }
                ));

        this->build_pbft();
        bzn::json_message msg;
        msg["bzn-api"] = "database";

        this->database_handler(msg, mock_session);
        EXPECT_NE(last_err, "");

        msg["msg"] = "not a valid crud message";
        this->database_handler(msg, mock_session);
        EXPECT_NE(last_err, "");

        bzn_msg payload;
        msg["msg"] = boost::beast::detail::base64_encode(payload.SerializeAsString());
        this->database_handler(msg, mock_session);
        EXPECT_EQ(last_err, "");
    }

    TEST_F(pbft_test, client_request_executed_results_in_message_response)
    {
        auto mock_session = std::make_shared<bzn::Mocksession_base>();
        EXPECT_CALL(*mock_session, send_datagram(_)).Times(Exactly(1));

        pbft_request msg;
        auto peers = std::make_shared<const std::vector<bzn::peer_address_t>>();
        auto op = std::make_shared<pbft_operation>(1, 1, msg, peers);
        op->set_session(mock_session);

        dummy_pbft_service service(this->mock_io_context);
        service.register_execute_handler([](auto, auto){});

        service.apply_operation(op);
    }

    TEST_F(pbft_test, messages_after_high_water_mark_dropped)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(0));

        this->preprepare_msg.set_sequence(pbft->get_high_water_mark() + 1);
        pbft->handle_message(preprepare_msg, default_original_msg);
    }

    TEST_F(pbft_test, messages_before_low_water_mark_dropped)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(0));

        this->preprepare_msg.set_sequence(this->pbft->get_low_water_mark());
        pbft->handle_message(preprepare_msg, default_original_msg);
    }

    TEST_F(pbft_test, request_redirect_to_primary_notifies_failure_detector) {
        EXPECT_CALL(*mock_failure_detector, request_seen(_)).Times(Exactly(0));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        EXPECT_FALSE(pbft->is_primary());
        pbft->handle_database_message(this->request_json, this->mock_session);
    }
}
