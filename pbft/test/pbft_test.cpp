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
#include <pbft/pbft_base.hpp>
#include <pbft/pbft.hpp>
#include <pbft/pbft_service.hpp>
#include <bootstrap/bootstrap_peers.hpp>
#include <mocks/mock_node_base.hpp>
#include <proto/bluzelle.pb.h>
#include <json/json.h>
#include <set>
#include <google/protobuf/text_format.h>
#include <boost/beast/core/detail/base64.hpp>

using namespace ::testing;

namespace
{

    const bzn::uuid_t TEST_NODE_UUID{"uuid0"};
    const bzn::uuid_t SECOND_NODE_UUID{"uuid1"};

    const bzn::peers_list_t TEST_PEER_LIST{{  "127.0.0.1", 8081, 8881, "name1", "uuid1"}
                                           , {"127.0.0.1", 8082, 8882, "name2", "uuid2"}
                                           , {"127.0.0.1", 8083, 8883, "name3", "uuid3"}
                                           , {"127.0.0.1", 8084, 8884, "name4", TEST_NODE_UUID}};


    class pbft_test : public Test
    {
    public:
        pbft_msg request_msg;
        pbft_msg preprepare_msg;

        std::shared_ptr<bzn::Mocknode_base> mock_node;
        std::shared_ptr<bzn::pbft_service> service;
        bzn::pbft pbft;

        pbft_test()
                : mock_node(std::make_shared<NiceMock<bzn::Mocknode_base>>())
                , service(std::make_shared<NiceMock<bzn::pbft_service>>())
                , pbft(mock_node, TEST_PEER_LIST, TEST_NODE_UUID, this->service)
        {
            request_msg.mutable_request()->set_operation("do some stuff");
            request_msg.mutable_request()->set_client("bob");
            request_msg.mutable_request()->set_timestamp(1);
            request_msg.set_type(PBFT_MSG_TYPE_REQUEST);

            preprepare_msg = pbft_msg(request_msg);
            preprepare_msg.set_type(PBFT_MSG_TYPE_PREPREPARE);
            preprepare_msg.set_sequence(19);
            preprepare_msg.set_view(1);
        }
    };


    pbft_msg
    extract_pbft_msg(std::shared_ptr<bzn::message> json)
    {
        pbft_msg result;
        result.ParseFromString(boost::beast::detail::base64_decode((*json)["pbft-data"].asString()));
        return result;
    }


    TEST_F(pbft_test, test_requests_create_operations)
    {
        ASSERT_EQ(0u, pbft.outstanding_operations_count());
        pbft.handle_message(request_msg);
        ASSERT_EQ(1u, pbft.outstanding_operations_count());
    }

    bool
    is_preprepare(std::shared_ptr<bzn::message> json)
    {
        pbft_msg msg = extract_pbft_msg(json);

        return msg.type() == PBFT_MSG_TYPE_PREPREPARE && msg.view() > 0 && msg.sequence() > 0;
    }


    TEST_F(pbft_test, test_requests_fire_preprepare)
    {
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_preprepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        pbft.handle_message(request_msg);
    }


    TEST_F(pbft_test, test_ignored_when_not_primary)
    {
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_preprepare, Eq(true))))
                .Times(Exactly(0));

        bzn::pbft pbft2(this->mock_node, TEST_PEER_LIST, SECOND_NODE_UUID, this->service);

        EXPECT_TRUE(this->pbft.is_primary());
        EXPECT_FALSE(pbft2.is_primary());
        pbft2.handle_message(request_msg);
    }

    std::set<uint64_t> seen_sequences;

    void
    save_sequences(const boost::asio::ip::tcp::endpoint& /*ep*/, std::shared_ptr<bzn::message> json)
    {
        pbft_msg msg = extract_pbft_msg(json);
        seen_sequences.insert(msg.sequence());
    }


    TEST_F(pbft_test, test_different_requests_get_different_sequences)
    {
        EXPECT_CALL(*mock_node, send_message(_, _)).WillRepeatedly(Invoke(save_sequences));

        pbft_msg request_msg2(request_msg);
        request_msg2.mutable_request()->set_timestamp(2);

        seen_sequences = std::set<uint64_t>();
        pbft.handle_message(request_msg);
        pbft.handle_message(request_msg2);
        ASSERT_EQ(seen_sequences.size(), 2u);
    }

    bool
    is_prepare(std::shared_ptr<bzn::message> json)
    {
        pbft_msg msg = extract_pbft_msg(json);

        return msg.type() == PBFT_MSG_TYPE_PREPARE && msg.view() > 0 && msg.sequence() > 0;
    }


    TEST_F(pbft_test, test_preprepare_triggers_prepare)
    {
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->pbft.handle_message(this->preprepare_msg);
    }


    TEST_F(pbft_test, test_prepare_contains_uuid)
    {
        std::shared_ptr<bzn::message> json;
        EXPECT_CALL(*mock_node, send_message(_, _)).WillRepeatedly(SaveArg<1>(&json));

        this->pbft.handle_message(this->preprepare_msg);

        pbft_msg msg_sent = extract_pbft_msg(json);

        ASSERT_EQ(msg_sent.sender(), this->pbft.get_uuid());
        ASSERT_EQ(msg_sent.sender(), TEST_NODE_UUID);
    }


    TEST_F(pbft_test, test_wrong_view_preprepare_rejected)
    {
        EXPECT_CALL(*mock_node, send_message(_, _)).Times(Exactly(0));

        pbft_msg preprepare2(this->preprepare_msg);
        preprepare2.set_view(6);

        this->pbft.handle_message(preprepare2);
    }


    TEST_F(pbft_test, test_no_duplicate_prepares_same_sequence_number)
    {
        EXPECT_CALL(*mock_node, send_message(_, _)).Times(Exactly(TEST_PEER_LIST.size()));

        pbft_msg prepreparea(this->preprepare_msg);
        pbft_msg preprepareb(this->preprepare_msg);
        pbft_msg prepreparec(this->preprepare_msg);
        pbft_msg preprepared(this->preprepare_msg);

        preprepareb.mutable_request()->set_timestamp(99);
        prepreparec.mutable_request()->set_operation("somethign else");
        preprepared.mutable_request()->set_client("certainly not bob");

        this->pbft.handle_message(prepreparea);
        this->pbft.handle_message(preprepareb);
        this->pbft.handle_message(prepreparec);
        this->pbft.handle_message(preprepared);
    }

    bool
    is_commit(std::shared_ptr<bzn::message> json)
    {
        pbft_msg msg = extract_pbft_msg(json);

        return msg.type() == PBFT_MSG_TYPE_COMMIT && msg.view() > 0 && msg.sequence() > 0;
    }


    TEST_F(pbft_test, test_commit_messages_sent)
    {
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_prepare, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(is_commit, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->pbft.handle_message(this->preprepare_msg);
        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg prepare = pbft_msg(this->preprepare_msg);
            prepare.set_type(PBFT_MSG_TYPE_PREPARE);
            prepare.set_sender(peer.uuid);
            this->pbft.handle_message(prepare);
        }
    }


    TEST_F(pbft_test, test_commits_applied)
    {

        pbft_msg preprepare = pbft_msg(this->preprepare_msg);
        preprepare.set_sequence(1);
        this->pbft.handle_message(preprepare);

        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg prepare = pbft_msg(preprepare);
            pbft_msg commit = pbft_msg(preprepare);
            prepare.set_type(PBFT_MSG_TYPE_PREPARE);
            prepare.set_sender(peer.uuid);
            commit.set_type(PBFT_MSG_TYPE_COMMIT);
            commit.set_sender(peer.uuid);
            this->pbft.handle_message(prepare);
            this->pbft.handle_message(commit);
        }

        EXPECT_EQ(this->service->applied_requests_count(), 1u);
    }

}
