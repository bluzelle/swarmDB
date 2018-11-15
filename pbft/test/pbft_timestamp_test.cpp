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

        auto create = new database_create;
        create->set_key(std::string("key"));
        create->set_value(std::string("value"));

        auto dmsg = new database_msg;
        dmsg->set_allocated_create(create);

        auto request = new pbft_request();
        request->set_type(PBFT_REQ_DATABASE);
        request->set_allocated_operation(dmsg);
        request->set_timestamp(this->now());
        request->set_client(TEST_NODE_UUID);

        bzn::json_message json_msg;
        json_msg["msg"] = request->SerializeAsString();

        // the first time we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(*request, json_msg);

        auto request2 = new pbft_request(*request);
        auto json_msg2 = json_msg;

        // this time no pre-prepare should be issued
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(0));
        this->handle_request(*request2, json_msg2);
    }

    TEST_F(pbft_proto_test, similar_request_generates_preprepare)
    {
        this->build_pbft();

        // send an initial message
        auto create = new database_create;
        create->set_key(std::string("key"));
        create->set_value(std::string("value"));

        auto dmsg = new database_msg;
        dmsg->set_allocated_create(create);

        auto request = new pbft_request();
        request->set_type(PBFT_REQ_DATABASE);
        request->set_allocated_operation(dmsg);
        request->set_timestamp(this->now());
        request->set_client(TEST_NODE_UUID);

        bzn::json_message json_msg;
        json_msg["msg"] = request->SerializeAsString();

        // we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(*request, json_msg);


        // send a second message the same as first but with a slightly different timestamp
        auto request2 = new pbft_request(*request);
        request2->set_timestamp(request2->timestamp() + 1);
        bzn::json_message json_msg2;
        json_msg2["msg"] = request2->SerializeAsString();

        // again we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(*request2, json_msg2);


        // send a third message the same as first but with same timestamp and different operation
        auto create3 = new database_create;
        create3->set_key(std::string("key3"));
        create3->set_value(std::string("value3"));

        auto dmsg3 = new database_msg;
        dmsg3->set_allocated_create(create3);
        auto request3 = new pbft_request(*request);
        request3->set_allocated_operation(dmsg3);
        bzn::json_message json_msg3;
        json_msg3["msg"] = request3->SerializeAsString();

        // again we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(*request3, json_msg3);
    }

    TEST_F(pbft_proto_test, same_request_from_different_client_generates_preprepare)
    {
        this->build_pbft();

        // send an initial message
        auto create = new database_create;
        create->set_key(std::string("key"));
        create->set_value(std::string("value"));

        auto dmsg = new database_msg;
        dmsg->set_allocated_create(create);

        auto request = new pbft_request();
        request->set_type(PBFT_REQ_DATABASE);
        request->set_allocated_operation(dmsg);
        request->set_timestamp(this->now());
        request->set_client(TEST_NODE_UUID);

        bzn::json_message json_msg;
        json_msg["msg"] = request->SerializeAsString();

        // we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(*request, json_msg);

        // send a second message the same as first but from different client
        auto request2 = new pbft_request(*request);
        request2->set_client(SECOND_NODE_UUID);
        bzn::json_message json_msg2;
        json_msg2["msg"] = request2->SerializeAsString();

        // again we should get pre-prepare messages
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
        this->handle_request(*request2, json_msg2);
    }

    TEST_F(pbft_proto_test, old_request_is_rejected)
    {
        this->build_pbft();

        auto create = new database_create;
        create->set_key(std::string("key"));
        create->set_value(std::string("value"));

        auto dmsg = new database_msg;
        dmsg->set_allocated_create(create);

        auto request = new pbft_request();
        request->set_type(PBFT_REQ_DATABASE);
        request->set_allocated_operation(dmsg);
        request->set_timestamp(this->now() - MAX_REQUEST_AGE_MS - 1);
        request->set_client(TEST_NODE_UUID);

        bzn::json_message json_msg;
        json_msg["msg"] = request->SerializeAsString();

        // we should NOT get pre-prepare messages since this is an old request
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(0));
        this->handle_request(*request, json_msg);
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