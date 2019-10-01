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
#include <pbft/operations/pbft_memory_operation.hpp>

namespace bzn::test
{

    TEST_F(pbft_test, test_requests_create_operations)
    {
        this->build_pbft();
        ASSERT_TRUE(pbft->is_primary());
        ASSERT_EQ(0u, this->operation_manager->held_operations_count());
        pbft->handle_database_message(this->request_msg, this->mock_session);
        ASSERT_EQ(1u, this->operation_manager->held_operations_count());
    }

    TEST_F(pbft_test, test_requests_fire_preprepare)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(is_preprepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        pbft->handle_database_message(this->request_msg, this->mock_session);
    }

    TEST_F(pbft_test, test_forwarded_to_primary_when_not_primary)
    {
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), A<std::shared_ptr<bzn_envelope>>())).Times(1).WillRepeatedly(Invoke(
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
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), _)).WillRepeatedly(Invoke(save_sequences));

        database_msg req, req2;
        req.mutable_header()->set_nonce(5);
        req2.mutable_header()->set_nonce(1055);

        seen_sequences = std::set<uint64_t>();
        pbft->handle_database_message(wrap_request(req), this->mock_session);
        pbft->handle_database_message(wrap_request(req2), this->mock_session);
        ASSERT_EQ(seen_sequences.size(), 2u);
    }

    TEST_F(pbft_test, test_preprepare_triggers_prepare)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->pbft->handle_message(this->preprepare_msg, default_original_msg);
    }

    TEST_F(pbft_test, test_wrong_view_preprepare_rejected)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), _)).Times(Exactly(0));

        pbft_msg preprepare2(this->preprepare_msg);
        preprepare2.set_view(6);

        this->pbft->handle_message(preprepare2, default_original_msg);
    }

    TEST_F(pbft_test, test_no_duplicate_prepares_same_sequence_number)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), _)).Times(Exactly(TEST_PEER_LIST.size()));

        pbft_msg preprepare2(this->preprepare_msg);
        preprepare2.set_request_hash("some other hash");

        this->pbft->handle_message(this->preprepare_msg, default_original_msg);
        this->pbft->handle_message(preprepare2, default_original_msg);
    }

    TEST_F(pbft_test, test_commit_messages_sent)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(is_commit, Eq(true))))
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

    TEST_F(pbft_test, client_request_does_not_result_in_message_ack)
    {
        this->build_pbft();
        auto mock_session = std::make_shared<NiceMock<bzn::mock_session_base>>();

        EXPECT_CALL(*mock_session, send_message(A<std::shared_ptr<std::string>>())).Times(Exactly(0));

        this->database_handler(this->request_msg, mock_session);
    }

    TEST_F(pbft_test, database_response_is_forwarded_to_session)
    {
        this->build_pbft();
        auto mock_session = std::make_shared<NiceMock<bzn::mock_session_base>>();

        EXPECT_CALL(*mock_session, send_message(A<std::shared_ptr<std::string>>())).Times(Exactly(1));

        this->pbft->sessions_waiting_on_forwarded_requests["utest"] = mock_session;

        database_response resp;
        resp.mutable_header()->set_request_hash("utest");

        this->request_msg.set_database_response(resp.SerializeAsString());
        this->database_response_handler(this->request_msg, mock_session);
    }

    TEST_F(pbft_test, add_session_to_sessions_waiting_can_add_a_session_and_shutdown_handler_removes_session_from_sessions_waiting)
    {
        this->build_pbft();

        EXPECT_EQ(size_t(0), this->pbft->sessions_waiting_on_forwarded_requests.size());

        bzn::session_shutdown_handler shutdown_handler{0};

        EXPECT_CALL(*mock_session, add_shutdown_handler(_))
            .Times(Exactly(1))
            .WillRepeatedly(Invoke([&](auto handler) {
                shutdown_handler = handler;
            }));

        pbft->handle_database_message(this->request_msg, this->mock_session);

        EXPECT_EQ(size_t(1), this->pbft->sessions_waiting_on_forwarded_requests.size());

        EXPECT_TRUE(shutdown_handler != nullptr);

        shutdown_handler();

        EXPECT_EQ(size_t(0), this->pbft->sessions_waiting_on_forwarded_requests.size());
    }

    TEST_F(pbft_test, client_request_executed_results_in_message_response)
    {
        auto mock_session = std::make_shared<bzn::mock_session_base>();
        EXPECT_CALL(*mock_session, send_message(A<std::shared_ptr<std::string>>())).Times(Exactly(1));

        auto peers = std::make_shared<const std::vector<bzn::peer_address_t>>();
        auto op = std::make_shared<pbft_memory_operation>(1, 1, "somehash", peers);
        op->set_session(mock_session);

        dummy_pbft_service service(this->mock_io_context);
        service.register_execute_handler([](auto){});

        service.apply_operation(op);
    }

    TEST_F(pbft_test, messages_before_low_water_mark_dropped)
    {
        this->build_pbft();
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(0));

        this->preprepare_msg.set_sequence(this->pbft->get_low_water_mark());
        pbft->handle_message(preprepare_msg, default_original_msg);
    }

    MATCHER(operation_ptr_has_session, "")
    {
        return arg->has_session();
    }

    TEST_F(pbft_test, request_redirect_attaches_session)
    {
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

    TEST_F(pbft_test, late_request_redirect_attaches_session)
    {
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


    TEST_F(pbft_test, bzn_envelope_has_repeated_request_body_field)
    {
        // My goal here is to generate a bzn_envelope and ensure that it has the repeated request_body field
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), A<std::shared_ptr<bzn_envelope>>())).Times(1).WillRepeatedly(Invoke(
                [&](auto /*ep*/, auto msg)
                {
                    EXPECT_EQ( 0, msg->piggybacked_requests_size());
                    auto piggybacked_requests = msg->mutable_piggybacked_requests();
                    EXPECT_TRUE(piggybacked_requests->empty());

                    bzn_envelope env;
                    *(msg->add_piggybacked_requests()) = env;
                    *(msg->add_piggybacked_requests()) = env;

                    EXPECT_FALSE(piggybacked_requests->empty());
                    EXPECT_EQ( 2, msg->piggybacked_requests_size());
                }));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();
        EXPECT_FALSE(pbft->is_primary());

        pbft->handle_database_message(this->request_msg, this->mock_session);
    }


    TEST_F(pbft_test, ensure_save_all_requests_records_requests)
    {
        EXPECT_CALL(*mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), A<std::shared_ptr<bzn_envelope>>())).Times(1).WillRepeatedly(Invoke(
                [&](auto /*ep*/, auto msg)
                {
                    EXPECT_EQ( 0, msg->piggybacked_requests_size());
                    auto piggybacked_requests = msg->mutable_piggybacked_requests();
                    EXPECT_TRUE(piggybacked_requests->empty());

                    bzn_envelope env;
                    pbft_msg d_msg;
                    env.set_pbft(d_msg.SerializeAsString());

                    *(msg->add_piggybacked_requests()) = env;

                    d_msg.set_view(3);
                    env.set_pbft(d_msg.SerializeAsString());

                    *(msg->add_piggybacked_requests()) = env;

                    auto hash_to_env = this->pbft->map_request_to_hash(*msg);

                    EXPECT_FALSE(piggybacked_requests->empty());
                    EXPECT_EQ( 2u, hash_to_env.size());
                }));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        this->pbft->handle_database_message(this->request_msg, this->mock_session);
    }
}
