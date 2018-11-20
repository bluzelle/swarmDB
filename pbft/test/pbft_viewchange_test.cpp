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
#include <pbft/test/pbft_proto_test.hpp>

namespace bzn
{

    class pbft_viewchange_test : public pbft_proto_test
    {
    public:

        size_t
        max_faulty_replicas_allowed() { return TEST_PEER_LIST.size() / 3; }

        void
        execute_handle_failure_expect_sut_to_send_viewchange()
        {
            // I expect that a replica forced to handle a failure will invalidate
            // its' view, and cause the replica to send a VIEWCHANGE messsage
            EXPECT_CALL(*mock_node, send_message_str(_, _))
                    .WillRepeatedly(Invoke([&](const auto & /*endpoint*/, const auto encoded_message) {
                        bzn_envelope envelope;
                        envelope.ParseFromString(*encoded_message);



                        pbft_msg view_change;
                        view_change.ParseFromString(envelope.pbft());
                        EXPECT_EQ(PBFT_MSG_VIEWCHANGE, view_change.type());
                        EXPECT_TRUE(2 == view_change.view());
                        EXPECT_TRUE(this->pbft->latest_stable_checkpoint().first == view_change.sequence());





                    }));
            this->pbft->handle_failure();
        }

        void
        check_that_pbft_drops_messages()
        {
            // We do not expect the pre-prepares due to the handled message at
            // the end of the test.
            EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_preprepare, Eq(true))))
                    .Times(Exactly(0));

            // nothing will happen with this request, that is there will be no new messages
            pbft->handle_message(this->preprepare_msg, default_original_msg);
        }

        void
        send_some_viewchange_messages(size_t n, bool(*f)(std::shared_ptr<std::string> wrapped_msg))
        {
            size_t count{0};
            EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(f, Eq(true))))
                    .Times(Exactly(TEST_PEER_LIST.size()))
                    .WillRepeatedly(Invoke([&](auto &, auto &) {


                        EXPECT_EQ( n, count);




                    }));

            pbft_msg pbft_msg;
            pbft_msg.set_type(PBFT_MSG_VIEWCHANGE);
            pbft_msg.set_view(this->pbft->get_view() + 1);

            // let's pretend that the sytem under test is receiving view change messages
            // from the other replicas
            for (const auto &peer : TEST_PEER_LIST)
            {
                if(peer.uuid == this->uuid)
                {
                    continue;
                }


                count++;

                this->pbft->handle_message(pbft_msg, this->default_original_msg);
                if (count == n)
                    break;
            }
        }


        void
        set_checkpoint(uint64_t sequence)
        {
            for (const auto &peer : TEST_PEER_LIST)
            {
                pbft_msg msg;
                msg.set_type(PBFT_MSG_CHECKPOINT);
                msg.set_view(this->pbft->get_view());
                msg.set_sequence(sequence);
                msg.set_state_hash(std::to_string(sequence));


                wrapped_bzn_msg wmsg;
                wmsg.set_type(BZN_MSG_PBFT);
                wmsg.set_sender(peer.uuid);
                // wmsg.set_signature(???)
                wmsg.set_payload(msg.SerializeAsString());

                this->pbft->handle_message(msg, this->default_original_msg);
            }
        }
    };


    TEST_F(pbft_viewchange_test, is_valid_viewchange_message)
    {
        this->build_pbft();

        bzn_envelope envelope;
        pbft_msg viewchange;

        // empty viewchange/envelope must be invalid.
        EXPECT_FALSE(this->pbft->is_valid_viewchange_message(viewchange, envelope));

        envelope.set_sender(TEST_PEER_LIST.begin()->uuid);


        EXPECT_TRUE(this->pbft->is_valid_viewchange_message(viewchange, envelope));









    }

    // void handle_viewchange      (const pbft_msg& msg, const bzn_envelope& original_msg);
    TEST_F(pbft_viewchange_test, primary_handle_viewchange)
    {
        this->build_pbft();

        pbft_msg msg;
        bzn_envelope original_msg;
        this->pbft->handle_viewchange(msg, original_msg);
    }


    TEST_F(pbft_viewchange_test, backup_handle_viewchange)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        pbft_msg msg;
        bzn_envelope original_msg;

        this->pbft->handle_viewchange(msg, original_msg);
    }


    TEST_F(pbft_viewchange_test, make_viewchange_makes_valid_message)
    {
        uint64_t new_view{4385967}; // next view number
        uint64_t n{44}; // n = sequence # of last valid checkpoint
        std::unordered_map<bzn::uuid_t, std::string> stable_checkpoint_proof
                {
                        std::pair<bzn::uuid_t, std::string> {"uuid_0","checkpoint_0"}
                        , std::pair<bzn::uuid_t, std::string> {"uuid_1","checkpoint_1"}
                        , std::pair<bzn::uuid_t, std::string> {"uuid_2","checkpoint_2"}
                        , std::pair<bzn::uuid_t, std::string> {"uuid_3","checkpoint_3"}
                        , std::pair<bzn::uuid_t, std::string> {"uuid_4","checkpoint_4"}
                };

        std::set<std::shared_ptr<bzn::pbft_operation>> prepared_operations;
        uuid_t sender{"uuid_0"};

        pbft_msg sut{pbft::make_viewchange(
                new_view
                , n
                , stable_checkpoint_proof
                , prepared_operations
        )};

        EXPECT_EQ(sut.type(), PBFT_MSG_VIEWCHANGE);
        EXPECT_EQ(sut.view(), new_view);

        std::unordered_map<bzn::uuid_t, std::string> sut_proofs;
        for ( uint8_t i = 0 ; i < sut.checkpoint_messages_size() ; ++i )
        {
            const auto c = sut.checkpoint_messages(i);
            LOG(debug) << c.length();
            //sut_proofs.insert
        }

        std::set<std::shared_ptr<bzn::pbft_operation>> sut_prepared_operations;
        for (uint8_t i = 0 ; i < sut.prepared_proofs_size() ; ++i )
        {
            LOG(debug) << sut.prepared_proofs(i).prepare_size();
        }


    }

    // TODO do we need this one too?
    TEST_F(pbft_viewchange_test, make_viewchange_message)
    {
        const uint64_t new_view_index = 13124;
        const uint64_t base_sequence_number = 9475;
        std::unordered_map<bzn::uuid_t, std::string> stable_checkpoint_proof;
        std::set<std::shared_ptr<bzn::pbft_operation>> prepared_operations;

        this->build_pbft();

        pbft_msg viewchange = this->pbft->make_viewchange(
                new_view_index
                , base_sequence_number
                , stable_checkpoint_proof
                , prepared_operations
        );

        EXPECT_EQ(PBFT_MSG_VIEWCHANGE, viewchange.type());
        EXPECT_EQ(new_view_index, viewchange.view());
        EXPECT_EQ(base_sequence_number, viewchange.sequence());

        // TODO stable_checkpoint_proof and prepared_operations

    }

    TEST_F(pbft_viewchange_test, test_prepared_operations_since_last_checkpoint)
    {
        // std::set<std::shared_ptr<bzn::pbft_operation>>
        // prepared_operations_since_last_checkpoint();
        this->build_pbft(true);

        EXPECT_EQ( (size_t)0, this->pbft->prepared_operations_since_last_checkpoint().size());











    }

    TEST_F(pbft_viewchange_test, test_make_viewchange_output)
    {
        this->build_pbft();

        size_t const oldview = this->pbft->get_view();

        for(size_t i{0} ; i < 99 ; ++i)
        {
            run_transaction_through_primary();
        }
        prepare_for_checkpoint(100);


        // expect checkpoint message
        run_transaction_through_primary();
        run_transaction_through_primary();
        run_transaction_through_primary();


        bzn_envelope viewchange_env;
        EXPECT_CALL(*mock_node, send_message_str(_, _))
                .WillRepeatedly(Invoke([&](const auto & /*endpoint*/, const auto encoded_message) {
                    viewchange_env.ParseFromString(*encoded_message);
                }));
        this->pbft->handle_failure();


        EXPECT_EQ(this->pbft->get_uuid(), viewchange_env.sender());

        pbft_msg viewchange;
        EXPECT_TRUE(viewchange.ParseFromString(viewchange_env.pbft()));

        EXPECT_EQ(PBFT_MSG_VIEWCHANGE, viewchange.type());

        EXPECT_EQ(uint64_t(oldview + 1), viewchange.view());

        EXPECT_EQ(uint64_t(103), viewchange.sequence());



        LOG(debug) << viewchange.SerializeAsString();

        EXPECT_TRUE(this->pbft->is_valid_viewchange_message(viewchange, viewchange_env));






    }










}