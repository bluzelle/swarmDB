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

        EXPECT_CALL(*(this->mock_node), register_for_message(bzn_envelope::kPbft, _))
                .Times(Exactly(1))
                .WillOnce(
                        Invoke(
                                [&](const auto&, auto handler)
                                {
                                    this->message_handler = handler;
                                    return true;
                                }
                        ));

        EXPECT_CALL(*(this->mock_node), register_for_message(bzn_envelope::kPbftMembership, _))
            .Times(Exactly(1))
            .WillOnce(
                Invoke(
                    [&](const auto&, auto handler)
                    {
                        this->membership_handler = handler;
                        return true;
                    }
                ));

        EXPECT_CALL(*(this->mock_node), register_for_message(bzn_envelope::kDatabaseMsg, _))
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

        database_msg db;
        this->request_msg.set_database_msg(db.SerializeAsString());

        preprepare_msg = pbft_msg();
        preprepare_msg.set_type(PBFT_MSG_PREPREPARE);
        preprepare_msg.set_sequence(19);
        preprepare_msg.set_view(1);
        preprepare_msg.set_allocated_request(new bzn_envelope(this->request_msg));
        preprepare_msg.set_request_hash(this->crypto->hash(this->request_msg));
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
                , this->crypto
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
    extract_pbft_msg(bzn_envelope msg)
    {
        pbft_msg result;
        result.ParseFromString(msg.pbft());
        return result;
    }

    std::string
    extract_sender(std::string msg)
    {
        bzn_envelope outer;
        outer.ParseFromString(msg);
        return outer.sender();
    }

    bzn_envelope
    wrap_pbft_msg(const pbft_msg& msg)
    {
        bzn_envelope result;
        result.set_pbft(msg.SerializeAsString());
        return result;
    }

    bzn_envelope
    wrap_pbft_membership_msg(const pbft_membership_msg& msg)
    {
        bzn_envelope result;
        result.set_pbft_membership(msg.SerializeAsString());
        return result;
    }

    bool
    is_preprepare(std::shared_ptr<bzn_envelope> wrapped_msg)
    {
        if (wrapped_msg->payload_case() != bzn_envelope::kPbft)
        {
            return false;
        }

        pbft_msg msg = extract_pbft_msg(*wrapped_msg);

        return msg.type() == PBFT_MSG_PREPREPARE && msg.view() > 0 && msg.sequence() > 0;
    }

    bool
    is_prepare(std::shared_ptr<bzn_envelope> wrapped_msg)
    {
        if (wrapped_msg->payload_case() != bzn_envelope::kPbft)
        {
            return false;
        }

        pbft_msg msg = extract_pbft_msg(*wrapped_msg);

        return msg.type() == PBFT_MSG_PREPARE && msg.view() > 0 && msg.sequence() > 0;
    }

    bool
    is_commit(std::shared_ptr<bzn_envelope> wrapped_msg)
    {
        if (wrapped_msg->payload_case() != bzn_envelope::kPbft)
        {
            return false;
        }

        pbft_msg msg = extract_pbft_msg(*wrapped_msg);

        return msg.type() == PBFT_MSG_COMMIT && msg.view() > 0 && msg.sequence() > 0;
    }

    bool
    is_checkpoint(std::shared_ptr<bzn_envelope> wrapped_msg)
    {
        if (wrapped_msg->payload_case() != bzn_envelope::kPbft)
        {
            return false;
        }

        pbft_msg msg = extract_pbft_msg(*wrapped_msg);

        return msg.type() == PBFT_MSG_CHECKPOINT && msg.sequence() > 0 && wrapped_msg->sender() != "" && msg.state_hash() != "";
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

    bzn_envelope
    from(uuid_t uuid)
    {
        bzn_envelope result;
        result.set_sender(uuid);
        return result;
    }

    bzn_envelope
    wrap_request(const database_msg& db)
    {
        bzn_envelope env;
        env.set_database_msg(db.SerializeAsString());

        return env;
    }
}
