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
#include <chrono>

using namespace ::testing;

namespace bzn
{
    using namespace test;

    TEST_F(pbft_proto_test, repeated_request_doesnt_generate_preprepare)
    {
        this->build_pbft();

        database_msg dmsg;
        dmsg.mutable_create()->set_key(std::string("key"));
        dmsg.mutable_create()->set_value(std::string("value"));

        bzn_envelope request;
        request.set_database_msg(dmsg.SerializeAsString());
        request.set_timestamp(this->now());
        request.set_sender(TEST_NODE_UUID);

        // the first time we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(request);

        auto request2 = bzn_envelope(request);

        // this time no pre-prepare should be issued
        EXPECT_CALL(*this->mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(0));
        std::shared_ptr<bzn::mock_session_base> session = std::make_shared<bzn::mock_session_base>();
        EXPECT_CALL(*session, send_message(ResultOf(test::is_swarm_error, Eq(true))));

        this->handle_request(request2, session);
    }

    TEST_F(pbft_proto_test, similar_request_generates_preprepare)
    {
        this->build_pbft();

        // send an initial message
        database_msg dmsg;
        dmsg.mutable_create()->set_key(std::string("key"));
        dmsg.mutable_create()->set_value(std::string("value"));

        bzn_envelope request;
        request.set_database_msg(dmsg.SerializeAsString());
        request.set_timestamp(this->now());
        request.set_sender(TEST_NODE_UUID);

        // we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(request);


        // send a second message the same as first but with a slightly different timestamp
        auto request2 = bzn_envelope(request);
        request2.set_timestamp(request2.timestamp() + 1);

        // again we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(request2);


        // send a third message the same as first but with same timestamp and different operation
        database_msg dmsg3;
        dmsg3.mutable_create()->set_key(std::string("key3"));
        dmsg3.mutable_create()->set_value(std::string("value3"));

        auto request3 = bzn_envelope(request);
        request3.set_database_msg(dmsg3.SerializeAsString());

        // again we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(request3);
    }

    TEST_F(pbft_proto_test, same_request_from_different_client_generates_preprepare)
    {
        this->build_pbft();

        // send an initial message
        database_msg dmsg;
        dmsg.mutable_create()->set_key(std::string("key"));
        dmsg.mutable_create()->set_value(std::string("value"));

        bzn_envelope request;
        request.set_database_msg(dmsg.SerializeAsString());
        request.set_timestamp(this->now());
        request.set_sender(TEST_NODE_UUID);

        // we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(request);

        // send a second message the same as first but from different client
        auto request2 = bzn_envelope(request);
        request2.set_sender(SECOND_NODE_UUID);

        // again we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(request2);
    }

    TEST_F(pbft_proto_test, old_request_is_rejected)
    {
        this->build_pbft();

        database_msg dmsg;
        dmsg.mutable_create()->set_key(std::string("key"));
        dmsg.mutable_create()->set_value(std::string("value"));

        bzn_envelope request;
        request.set_database_msg(dmsg.SerializeAsString());
        request.set_timestamp(0);
        request.set_sender(TEST_NODE_UUID);

        // we should NOT get pre-prepare messages since this is an old request
        EXPECT_CALL(*this->mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(0));

        std::shared_ptr<bzn::mock_session_base> session = std::make_shared<bzn::mock_session_base>();
        EXPECT_CALL(*session, send_message(ResultOf(test::is_swarm_error, Eq(true))));

        this->handle_request(request, session);
    }

    TEST_F(pbft_proto_test, range_test)
    {
        std::map<int, std::string> m;
        m.insert(std::make_pair(2, "2"));
        m.insert(std::make_pair(3, "3"));
        m.insert(std::make_pair(4, "4"));

        auto r = m.equal_range(1);
        EXPECT_EQ(r.first, r.second);
    }
}
