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

namespace bzn::test
{

    pbft_test::pbft_test()
    {
        // This pattern copied from audit_test, to allow us to declare expectations on the timer that pbft will
        // construct

        EXPECT_CALL(*(this->mock_node), register_for_message(bzn_msg_type::BZN_MSG_PBFT, _))
                .Times(Exactly(1))
                .WillOnce(
                        Invoke(
                                [&](const auto&, auto handler)
                                {
                                    this->message_handler = handler;
                                    return true;
                                }
                        ));

        EXPECT_CALL(*(this->mock_node), register_for_message("database", _))
                .Times(Exactly(1))
                .WillOnce(
                        Invoke(
                                [&](const auto&, auto handler)
                                {
                                    this->database_handler = handler;
                                    return true;
                                }
                        ));

        EXPECT_CALL(*(this->mock_io_context), make_unique_steady_timer())
                .Times(AtMost(1))
                .WillOnce(
                        Invoke(
                                [&]()
                                { return std::move(this->audit_heartbeat_timer); }
                        ));

        EXPECT_CALL(*(this->audit_heartbeat_timer), async_wait(_))
                .Times(AnyNumber())
                .WillRepeatedly(
                        Invoke(
                                [&](auto handler)
                                { this->audit_heartbeat_timer_callback = handler; }
                        ));

        EXPECT_CALL(*(this->mock_service), register_execute_handler(_))
                .Times(Exactly(1))
                .WillOnce(
                        Invoke(
                                [&](auto handler)
                                { this->service_execute_handler = handler; }
                        ));

//        request_msg.mutable_request()->set_operation("do some stuff");
        request_msg.mutable_request()->set_client("bob");
        request_msg.mutable_request()->set_timestamp(1);
        request_msg.set_type(PBFT_MSG_REQUEST);

        preprepare_msg = pbft_msg(request_msg);
        preprepare_msg.set_type(PBFT_MSG_PREPREPARE);
        preprepare_msg.set_sequence(19);
        preprepare_msg.set_view(1);
    }

    void
    pbft_test::build_pbft()
    {
        this->pbft = std::make_shared<bzn::pbft>(
                this->mock_node
                , this->mock_io_context
                , TEST_PEER_LIST
                , this->uuid
                , this->mock_service
                , this->mock_failure_detector
        );
        this->pbft->set_audit_enabled(false);
        this->pbft->start();
        this->pbft_built = true;
    }

    void
    pbft_test::TearDown()
    {
        // The code that extracts callbacks, etc expects that this will actually happen at some point, but some
        // of the tests do not actually require it
        if (!this->pbft_built)
        {
            this->build_pbft();
        }
    }

    pbft_msg
    extract_pbft_msg(std::string msg)
    {
        wrapped_bzn_msg outer;
        outer.ParseFromString(msg);
        pbft_msg result;
        result.ParseFromString(outer.payload());
        return result;
    }

    wrapped_bzn_msg
    wrap_pbft_msg(const pbft_msg& msg)
    {
        wrapped_bzn_msg result;
        result.set_payload(msg.SerializeAsString());
        result.set_type(bzn_msg_type::BZN_MSG_PBFT);
        return result;
    }

    bool
    is_preprepare(std::shared_ptr<std::string> wrapped_msg)
    {

        pbft_msg msg = extract_pbft_msg(*wrapped_msg);

        return msg.type() == PBFT_MSG_PREPREPARE && msg.view() > 0 && msg.sequence() > 0;
    }

    bool
    is_prepare(std::shared_ptr<std::string> wrapped_msg)
    {
        pbft_msg msg = extract_pbft_msg(*wrapped_msg);

        return msg.type() == PBFT_MSG_PREPARE && msg.view() > 0 && msg.sequence() > 0;
    }

    bool
    is_commit(std::shared_ptr<std::string> wrapped_msg)
    {
        pbft_msg msg = extract_pbft_msg(*wrapped_msg);

        return msg.type() == PBFT_MSG_COMMIT && msg.view() > 0 && msg.sequence() > 0;
    }

    bool
    is_checkpoint(std::shared_ptr<std::string> wrapped_msg)
    {
        pbft_msg msg = extract_pbft_msg(*wrapped_msg);

        return msg.type() == PBFT_MSG_CHECKPOINT && msg.sequence() > 0 && msg.sender() != "" && msg.state_hash() != "";
    }

    bool
    is_audit(std::shared_ptr<std::string> msg)
    {
        Json::Value json;
        Json::Reader reader;

        if (!reader.parse(*msg, json))
        {
            return false;
        }

        return json["bzn-api"] == "audit";
    }
}
