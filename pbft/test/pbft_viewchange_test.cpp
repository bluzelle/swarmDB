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
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_node_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_crypto_base.hpp>
#include <mocks/mock_options_base.hpp>
#include <pbft/test/pbft_proto_test.hpp>
#include <utils/make_endpoint.hpp>

namespace bzn
{
    class pbft_viewchange_test : public pbft_proto_test
    {
    public:
        std::shared_ptr<bzn::options_base> options = std::make_shared<bzn::options>();

        std::shared_ptr<Mockcrypto_base>
        build_pft_with_mock_crypto()
        {
            std::shared_ptr<Mockcrypto_base> mockcrypto = std::make_shared<Mockcrypto_base>();
            this->crypto = mockcrypto;
            this->build_pbft();
            return mockcrypto;
        }

        void
        run_transaction_through_primary_times(const size_t repeat, uint64_t& current_sequence)
        {
            for (size_t i{0}; i<repeat; ++i)
            {
                current_sequence++;
                run_transaction_through_primary(false);
            }
        }

        void
        generate_checkpoint_at_sequence_100(uint64_t& current_sequence)
        {
            auto mockcrypto = this->build_pft_with_mock_crypto();

            EXPECT_CALL(*mockcrypto, hash(An<const bzn_envelope&>()))
                    .WillRepeatedly(Invoke([&](const bzn_envelope& envelope)
                                           {
                                               return envelope.sender() + "_" + std::to_string(current_sequence) + "_" + std::to_string(envelope.timestamp());
                                           }));

            EXPECT_CALL(*mockcrypto, verify(_))
                    .WillRepeatedly(Invoke([&](const bzn_envelope& /*msg*/)
                                           {
                                               return true;
                                           }));

            for (current_sequence=1; current_sequence < 100; ++current_sequence)
            {
                run_transaction_through_primary();
            }
            prepare_for_checkpoint(current_sequence);
            run_transaction_through_primary();
            this->stabilize_checkpoint(current_sequence);
        }
    };

    TEST_F(pbft_viewchange_test, test_fill_in_missing_pre_prepares)
    {
        auto mock_crypto = this->build_pft_with_mock_crypto();

        EXPECT_CALL(*mock_crypto, sign(_))
                .WillRepeatedly(Invoke([&](bzn_envelope &msg)
                                 {
                                     msg.set_sender(this->pbft->get_uuid());
                                     msg.set_signature("mock_signature");
                                     return true;
                                 }));

        EXPECT_CALL(*mock_crypto, hash(An<const bzn_envelope&>()))
                .WillRepeatedly(Invoke([&](auto env) {return env.SerializeAsString();}));
        
        bzn_envelope envelope;
        std::map<uint64_t, bzn_envelope> pre_prepares;
        pre_prepares.insert(std::make_pair(uint64_t(103), envelope));
        pre_prepares.insert(std::make_pair(uint64_t(101), envelope));
        this->pbft->fill_in_missing_pre_prepares(99, 4, pre_prepares);
        EXPECT_TRUE(0 < pre_prepares.count(102));

        ///////////////////////////////////////                                                                 }));
        pre_prepares.insert(std::make_pair(uint64_t(107), envelope));
        this->pbft->fill_in_missing_pre_prepares(99, 4, pre_prepares);

        uint64_t sequence = 100;
        for (const auto pre_prepare : pre_prepares)
        {
            EXPECT_EQ(sequence , pre_prepare.first);
            ++sequence;
        }

        EXPECT_TRUE(0 == pre_prepares.count(sequence));
    }

    TEST_F(pbft_viewchange_test, pbft_with_invalid_view_drops_messages)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        this->pbft->handle_failure();

        // after handling the failure, the pbft must ignore all messages save for
        // checkpoint, view change and new view messages
        pbft_msg message;

        message.set_type(PBFT_MSG_PREPREPARE);
        EXPECT_FALSE(this->pbft->preliminary_filter_msg(message));

        message.set_type(PBFT_MSG_PREPARE);
        EXPECT_FALSE(this->pbft->preliminary_filter_msg(message));

        message.set_type(PBFT_MSG_COMMIT);
        EXPECT_FALSE(this->pbft->preliminary_filter_msg(message));

        message.set_type(PBFT_MSG_CHECKPOINT);
        EXPECT_TRUE(this->pbft->preliminary_filter_msg(message));

        message.set_type(PBFT_MSG_JOIN);
        EXPECT_FALSE(this->pbft->preliminary_filter_msg(message));

        message.set_type(PBFT_MSG_LEAVE);
        EXPECT_FALSE(this->pbft->preliminary_filter_msg(message));

        message.set_type(PBFT_MSG_VIEWCHANGE);
        EXPECT_TRUE(this->pbft->preliminary_filter_msg(message));

        message.set_type(PBFT_MSG_NEWVIEW);
        EXPECT_TRUE(this->pbft->preliminary_filter_msg(message));
    }

    TEST_F(pbft_viewchange_test, test_is_peer)
    {
        const bzn::peer_address_t NOT_PEER{"127.0.0.1", 9091, 9991, "not_a_peer", "uuid_nope"};
        this->build_pbft();

        for (const auto& peer : TEST_PEER_LIST)
        {
            EXPECT_TRUE(this->pbft->is_peer(peer.uuid));
        }

        EXPECT_FALSE(this->pbft->is_peer(NOT_PEER.uuid));
    }

    TEST_F(pbft_viewchange_test, validate_and_extract_checkpoint_hashes)
    {
        uint64_t current_sequence{0};
        generate_checkpoint_at_sequence_100(current_sequence);

        this->run_transaction_through_primary_times(2, current_sequence);

        EXPECT_CALL(*mock_node, send_message(_, ResultOf(test::is_viewchange, Eq(true))))
                .WillRepeatedly(Invoke(
                        [&](const auto & /*endpoint*/, const auto &viewchange_env)
                        {
                            pbft_msg viewchange;
                            viewchange.ParseFromString(viewchange_env->pbft());
                            EXPECT_EQ(PBFT_MSG_VIEWCHANGE, viewchange.type());

                            std::map<bzn::checkpoint_t, std::set<bzn::uuid_t>> checkpoints = this->pbft->validate_and_extract_checkpoint_hashes(
                                    viewchange);
                            EXPECT_EQ(uint64_t(1), checkpoints.size());

                            for (const auto &p : checkpoints)
                            {
                                auto checkpoint = p.first;
                                auto uuids = p.second;

                                // there will be a checkpoint 100, with a hash value of "100"
                                EXPECT_EQ(uint64_t(100), checkpoint.first);
                                EXPECT_EQ("100", checkpoint.second);

                                EXPECT_EQ(uint64_t(3), uuids.size());
                                for (const auto &uuid : uuids)
                                {
                                    EXPECT_FALSE(TEST_PEER_LIST.end() ==
                                                 std::find_if(TEST_PEER_LIST.begin(), TEST_PEER_LIST.end(),
                                                              [&](const auto &peer)
                                                              {
                                                                  return peer.uuid == uuid;
                                                              }));
                                }
                            }
                        }));
        this->pbft->handle_failure();
    }

    TEST_F(pbft_viewchange_test, test_is_valid_viewchange_message)
    {
        uint64_t current_sequence{0};

        generate_checkpoint_at_sequence_100(current_sequence);

        this->run_transaction_through_primary_times(2, current_sequence);


        EXPECT_CALL(*mock_node, send_message(_, ResultOf(test::is_viewchange, Eq(true))))
                .WillRepeatedly(Invoke([&](const auto & /*endpoint*/, const auto viewchange_env)
                {
                    EXPECT_EQ(this->pbft->get_uuid(), viewchange_env->sender());
                    pbft_msg viewchange;
                    EXPECT_TRUE(viewchange.ParseFromString(viewchange_env->pbft())); // this will be valid.
                    EXPECT_TRUE(this->pbft->is_valid_viewchange_message(viewchange, *viewchange_env));

                }));

        this->pbft->handle_failure();
    }

    TEST_F(pbft_viewchange_test, make_viewchange_makes_valid_message)
    {
        uint64_t current_sequence{0};

        generate_checkpoint_at_sequence_100(current_sequence);

        this->run_transaction_through_primary_times(2, current_sequence);

        auto viewchange = this->pbft->make_viewchange(this->pbft->get_view() + uint64_t(1), current_sequence, this->pbft->stable_checkpoint_proof, this->pbft->prepared_operations_since_last_checkpoint());

        EXPECT_EQ(PBFT_MSG_VIEWCHANGE, viewchange.type());
        EXPECT_EQ(current_sequence, viewchange.sequence());
        EXPECT_EQ(3, viewchange.checkpoint_messages_size());
    }

    TEST_F(pbft_viewchange_test, pbft_handle_failure_causes_invalid_view_state_and_starts_viewchange)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        EXPECT_CALL(*mock_node, send_message_str(_, _))
                .WillRepeatedly(Invoke([&](const auto & /*endpoint*/, const auto encoded_message)
                                       {
                                           bzn_envelope envelope;
                                           envelope.ParseFromString(*encoded_message);


                                           pbft_msg view_change;
                                           view_change.ParseFromString(envelope.pbft());
                                           EXPECT_EQ(PBFT_MSG_VIEWCHANGE, view_change.type());
                                           EXPECT_TRUE(2 == view_change.view());
                                           EXPECT_TRUE(this->pbft->latest_stable_checkpoint().first == view_change.sequence());
                                       }));

        this->pbft->handle_failure();

        // Now the replica's view should be invalid
        EXPECT_FALSE(this->pbft->is_view_valid());
    }

    TEST_F(pbft_viewchange_test, test_prepared_operations_since_last_checkpoint)
    {
        uint64_t current_sequence{0};

        generate_checkpoint_at_sequence_100(current_sequence);

        EXPECT_EQ(size_t(0), this->pbft->prepared_operations_since_last_checkpoint().size());

        run_transaction_through_primary(false);
        current_sequence++;
        EXPECT_EQ(size_t(1), this->pbft->prepared_operations_since_last_checkpoint().size());

        run_transaction_through_primary(false);
        current_sequence++;
        EXPECT_EQ(size_t(2), this->pbft->prepared_operations_since_last_checkpoint().size());

        auto operations = this->pbft->prepared_operations_since_last_checkpoint();
        for(const auto& operation : operations)
        {
            // TODO: what other tests?
            EXPECT_EQ(uint64_t(1), operation->get_view());
            EXPECT_TRUE(operation->get_sequence() > 100 && operation->get_sequence() <= current_sequence);
        }
    }

    TEST_F(pbft_viewchange_test, test_save_checkpoint)
    {
        auto mock_crypto = this->build_pft_with_mock_crypto();

        // I expect 3 calls from crypto::verify
        EXPECT_CALL(*mock_crypto, verify(_)).Times(3).WillRepeatedly(Invoke([&](const bzn_envelope& /*msg*/){return true;}));

        pbft_msg msg;

        // add three checkpoint messages
        for (uint64_t i{1}; i<4; ++i)
        {
            bzn_envelope checkpoint;
            checkpoint.set_sender("a_nice_sender");
            checkpoint.set_signature("signature_" + std::to_string(i));

            pbft_msg checkpoint_msg;
            checkpoint_msg.set_sequence(i);
            checkpoint_msg.set_state_hash("a_state_hash_" + std::to_string(i));

            checkpoint.set_pbft(checkpoint_msg.SerializeAsString());
            *(msg.add_checkpoint_messages()) = checkpoint;
        }
        this->pbft->save_checkpoint(msg);
        //  expect that there should be 3 unstable checkpoint proofs after calling save_checkpoint.
        EXPECT_EQ(uint64_t(3), this->pbft->unstable_checkpoint_proofs.size());
    }

    TEST_F(pbft_viewchange_test, test_handle_viewchange)
    {
        // get sut1 to generate viewchange message
        // catch message
        // send to sut2 handle_viewchange
        // sut2 should store viewchange
        // re-send from other nodes to handle_viewchange
        // sut2 should eventually send its own viewchange
        // once enough viewchange messages are sent, sut2 should send newview
        // capture newview and send to sut1 handle_newview

        // sut1 (this->pbft) is initial primary
        auto mock_crypto = this->build_pft_with_mock_crypto();

        // sut2 is new primary after view change
        std::shared_ptr<bzn::Mocknode_base> mock_node2 = std::make_shared<NiceMock<bzn::Mocknode_base>>();
        std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context2 =
                std::make_shared<NiceMock<bzn::asio::Mockio_context_base >>();
        std::unique_ptr<bzn::asio::Mocksteady_timer_base> audit_heartbeat_timer2 =
                std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base >>();
        std::shared_ptr<bzn::mock_pbft_service_base> mock_service2 =
                std::make_shared<NiceMock<bzn::mock_pbft_service_base>>();

        EXPECT_CALL(*(mock_io_context2), make_unique_steady_timer())
                .Times(AtMost(1))
                .WillOnce(
                        Invoke(
                                [&]()
                                { return std::move(audit_heartbeat_timer2); }
                        ));

        auto mock_options = std::make_shared<bzn::mock_options_base>();

        EXPECT_CALL(*mock_options, get_uuid()).WillRepeatedly(Invoke([](){return "uuid2";}));
        
        auto pbft2 = std::make_shared<bzn::pbft>(mock_node2, mock_io_context2, TEST_PEER_LIST, mock_options, mock_service2, this->mock_failure_detector, this->crypto);
        pbft2->set_audit_enabled(false);

        pbft2->start();

        EXPECT_CALL(*mock_crypto, hash(An<const bzn_envelope&>()))
                .WillRepeatedly(Invoke([&](auto env)
                                       {
                                           return env.SerializeAsString();
                                       }));

        EXPECT_CALL(*mock_crypto, verify(_))
                .WillRepeatedly(Invoke([&](const bzn_envelope& /*msg*/)
                                       {
                                           return true;
                                       }));


        EXPECT_CALL(*mock_crypto, sign(_))
                .WillRepeatedly(Invoke([&](const bzn_envelope& /*msg*/)
                                       {
                                           return true;
                                       }));


        // set up a stable checkpoint plus a couple of uncommitted transactions on sut1
        for (size_t i = 0; i < 99; i++)
        {
            run_transaction_through_primary();
        }
        prepare_for_checkpoint(100);
        run_transaction_through_primary();
        this->stabilize_checkpoint(100);
        run_transaction_through_primary(false);
        run_transaction_through_primary(false);

        for (auto const &p : TEST_PEER_LIST)
        {
            EXPECT_CALL(*(this->mock_node),
                        send_message(bzn::make_endpoint(p), ResultOf(test::is_viewchange, Eq(true))))
                    .Times(Exactly(1))
                    .WillRepeatedly(Invoke([&](auto, auto wmsg)
                                           {
                                               pbft_msg msg;
                                               ASSERT_TRUE(msg.ParseFromString(wmsg->pbft()));
                                               wmsg->set_sender(p.uuid);
                                               pbft2->stable_checkpoint = this->pbft->stable_checkpoint;
                                               pbft2->handle_viewchange(msg, *wmsg);
                                           }));
        }

        for (auto const &p : TEST_PEER_LIST)
        {
            EXPECT_CALL(*mock_node2, send_message(bzn::make_endpoint(p), ResultOf(test::is_newview, Eq(true))))
                    .Times(Exactly(1))
                    .WillRepeatedly(Invoke([&](auto, auto wmsg)
                                           {
                                                if (p.uuid == TEST_NODE_UUID)
                                               {
                                                   EXPECT_CALL(*this->mock_node, send_message(_, ResultOf(test::is_prepare, Eq(true))))
                                                           .Times(Exactly(2 * TEST_PEER_LIST.size()));
                                                   pbft_msg msg;
                                                   ASSERT_TRUE(msg.ParseFromString(wmsg->pbft()));
                                                   this->pbft->handle_newview(msg, *wmsg);
                                               }
                                           }));
        }

        EXPECT_CALL(*mock_node2, send_message(_, ResultOf(test::is_viewchange, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        // get sut1 to generate viewchange message
        this->pbft->handle_failure();

        EXPECT_EQ(this->pbft->view, 2U);
        EXPECT_EQ(pbft2->next_issued_sequence_number, 103U);
    }

    TEST_F(pbft_viewchange_test, is_valid_viewchange_fails_if_no_checkpoint_yet)
    {
        // This test was written for KEP-902: is_valid_viewchange throws if no checkpoint yet
        auto mock_crypto = this->build_pft_with_mock_crypto();
        bzn_envelope original_message;
        EXPECT_CALL(*mock_node, send_message(_, ResultOf(test::is_viewchange, Eq(true)))).WillRepeatedly(Invoke([&](const auto & /*endpoint*/, const auto &viewchange_env) { original_message = *viewchange_env; }));

        this->pbft->handle_failure();

        pbft_msg msg;
        msg.ParseFromString(original_message.pbft());
        EXPECT_FALSE(this->pbft->is_valid_viewchange_message(msg, original_message));
    }
}
