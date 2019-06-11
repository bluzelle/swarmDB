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

        EXPECT_CALL(*(this->mock_node), register_for_message(bzn_envelope::kDatabaseResponse, _))
            .Times(Exactly(1))
            .WillOnce(
                Invoke(
                    [&](const auto&, auto handler)
                    {
                        this->database_response_handler = handler;
                        return true;
                    }
                ));

        EXPECT_CALL(*(this->mock_node), register_for_message(bzn_envelope::kCheckpointMsg, _))
                .Times(Exactly(1))
                .WillOnce(
                        Invoke(
                                [&](const auto&, auto handler)
                                {
                                    this->checkpoint_msg_handler = handler;
                                    return true;
                                }
                        ));


        EXPECT_CALL(*(this->mock_io_context), make_unique_steady_timer())
                .Times(AnyNumber())
                .WillOnce(
                        Invoke(
                                [&]()
                                { return std::move(this->audit_heartbeat_timer); }
                        ))
                .WillOnce(
                    Invoke(
                        [&]()
                        { return std::move(this->new_config_timer); }
                    ))
                .WillOnce(
                    Invoke(
                        [&]()
                        { return std::move(this->join_retry_timer); }
                    ))
                .WillRepeatedly(
                    Invoke(
                        [&]()
                        {
                            auto timer = std::make_unique<NiceMock<bzn::asio::mock_steady_timer_base >>();
                            EXPECT_CALL(*timer, async_wait(_))
                                .Times(AnyNumber())
                                .WillOnce(
                                    Invoke(
                                        [&](const auto handler)
                                        {
                                            this->cp_manager_timer_callbacks[cp_manager_timer_callback_count++] = handler;
                                        }
                                    )
                                );
                            return timer;
                        }
                    ));

        EXPECT_CALL(*(this->audit_heartbeat_timer), async_wait(_))
                .Times(AnyNumber())
                .WillRepeatedly(
                        Invoke(
                                [&](auto handler)
                                { this->audit_heartbeat_timer_callback = handler; }
                        ));

        EXPECT_CALL(*(this->new_config_timer), async_wait(_))
                .Times(AnyNumber())
                .WillRepeatedly(
                        Invoke(
                            [&](auto handler)
                            { this->new_config_timer_callback = handler; }
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

        this->options->get_mutable_simple_options().set("listener_address", TEST_NODE_ADDR);
        this->options->get_mutable_simple_options().set("listener_port", std::to_string(TEST_NODE_LISTEN_PORT));
        this->options->get_mutable_simple_options().set("crypto_enabled_incoming", std::to_string(false));
        this->options->get_mutable_simple_options().set("crypto_enabled_outgoing", std::to_string(false));

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
        this->options->get_mutable_simple_options().set("uuid", this->uuid);
        this->pbft = std::make_shared<bzn::pbft>(
                this->mock_node
                , this->mock_io_context
                , TEST_PEER_LIST
                , this->options
                , this->mock_service
                , this->mock_failure_detector
                , this->crypto
                , this->operation_manager
                , this->storage
                , this->monitor
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

    void
    pbft_test::send_preprepare(uint64_t view, uint64_t sequence, bzn::hash_t req_hash, std::optional<bzn_envelope> request)
    {
        pbft_msg preprepare;
        preprepare.set_view(view);
        preprepare.set_sequence(sequence);
        preprepare.set_request_hash(req_hash);
        preprepare.set_type(PBFT_MSG_PREPREPARE);

        if (request)
        {
            preprepare.set_allocated_request(new bzn_envelope(*request));
        }

        bzn_envelope original;
        original.set_pbft(preprepare.SerializeAsString());

        this->pbft->handle_message(preprepare, original);
    }

    void
    pbft_test::send_prepares(uint64_t view, uint64_t sequence, bzn::hash_t req_hash)
    {
        pbft_msg prepare;
        prepare.set_view(view);
        prepare.set_sequence(sequence);
        prepare.set_request_hash(req_hash);
        prepare.set_type(PBFT_MSG_PREPARE);

        for (const auto& peer : TEST_PEER_LIST)
        {
            bzn_envelope original;
            original.set_pbft(prepare.SerializeAsString());
            original.set_sender(peer.uuid);
            this->pbft->handle_message(prepare, original);
        }
    }

    void
    pbft_test::send_commits(uint64_t view, uint64_t sequence, bzn::hash_t req_hash)
    {
        pbft_msg commit;
        commit.set_view(view);
        commit.set_sequence(sequence);
        commit.set_request_hash(req_hash);
        commit.set_type(PBFT_MSG_COMMIT);

        for (const auto& peer : TEST_PEER_LIST)
        {
            bzn_envelope original;
            original.set_pbft(commit.SerializeAsString());
            original.set_sender(peer.uuid);
            this->pbft->handle_message(commit, original);
        }
    }

    uint64_t now()
    {
        return (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }

    pbft_msg
    extract_pbft_msg(bzn_envelope msg)
    {
        pbft_msg result;
        result.ParseFromString(msg.pbft());
        return result;
    }

    pbft_membership_msg
    extract_pbft_membership_msg(bzn_envelope msg)
    {
        pbft_membership_msg result;
        result.ParseFromString(msg.pbft_membership());
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
    wrap_pbft_msg(const pbft_msg& msg, const bzn::uuid_t sender)
    {
        bzn_envelope result;
        result.set_pbft(msg.SerializeAsString());
        result.set_sender(sender);
        return result;
    }

    bzn_envelope
    wrap_pbft_membership_msg(const pbft_membership_msg& msg, const bzn::uuid_t sender)
    {
        bzn_envelope result;
        result.set_pbft_membership(msg.SerializeAsString());
        result.set_sender(sender);
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
        if (wrapped_msg->payload_case() != bzn_envelope::kCheckpointMsg)
        {
            return false;
        }

        checkpoint_msg msg;

        return msg.ParseFromString(wrapped_msg->checkpoint_msg()) && msg.sequence() != 0 && msg.state_hash() != "";
    }

    bool
    is_join(std::shared_ptr<bzn_envelope> wrapped_msg)
    {
        if (wrapped_msg->payload_case() != bzn_envelope::kPbftMembership)
        {
            return false;
        }

        auto msg = extract_pbft_membership_msg(*wrapped_msg);

        return msg.type() == PBFT_MMSG_JOIN && wrapped_msg->sender() != "";
    }

    bool
    is_audit(std::shared_ptr<bzn_envelope> msg)
    {
        audit_message parsed;
        return (msg->payload_case() == bzn_envelope::kAudit && parsed.ParseFromString(msg->audit()));
    }

    bool
    is_viewchange(std::shared_ptr<bzn_envelope> wrapped_msg)
    {
        pbft_msg msg;
        msg.ParseFromString(wrapped_msg->pbft());
        return msg.type() == PBFT_MSG_VIEWCHANGE;
    }

    bool
    is_newview(std::shared_ptr<bzn_envelope> wrapped_msg)
    {
        pbft_msg msg;
        msg.ParseFromString(wrapped_msg->pbft());
        return msg.type() == PBFT_MSG_NEWVIEW;
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
