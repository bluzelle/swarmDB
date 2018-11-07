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

namespace
{

};


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
                    .WillRepeatedly(Invoke([&](const auto & /*endpoint*/, const auto p) {
                        wrapped_bzn_msg wmsg;
                        wmsg.ParseFromString(*p);
                        pbft_msg view_change;
                        view_change.ParseFromString(wmsg.payload());
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
                pbft_msg.set_sender(peer.uuid);
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
                msg.set_sender(peer.uuid);


                wrapped_bzn_msg wmsg;
                wmsg.set_type(BZN_MSG_PBFT);
                wmsg.set_sender(peer.uuid);
                // wmsg.set_signature(???)
                wmsg.set_payload(msg.SerializeAsString());



                this->pbft->handle_message(msg, this->default_original_msg);
            }
        }

    };

    TEST_F(pbft_viewchange_test, pbft_handle_failure_causes_invalid_view_state_and_starts_viewchange)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        this->execute_handle_failure_expect_sut_to_send_viewchange();

        // Now the replica's view should be invalid
        EXPECT_FALSE(this->pbft->is_view_valid());
    }

    TEST_F(pbft_viewchange_test, pbft_with_invalid_view_drops_messages)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();
        this->execute_handle_failure_expect_sut_to_send_viewchange();
        this->check_that_pbft_drops_messages();
    }

    TEST_F(pbft_viewchange_test, pbft_no_history_replica_sends_viewchange_message_after_receiving_enough_viewchange_messages)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();
        this->send_some_viewchange_messages(this->max_faulty_replicas_allowed() + 1, is_viewchange);
    }

    TEST_F(pbft_viewchange_test, pbft_replica_with_history_sends_viewchange_message)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        for (size_t i = 0; i < 99; i++)
        {
            run_transaction_through_primary();
        }
        prepare_for_checkpoint(100);
        run_transaction_through_primary();
        stabilize_checkpoint(100);






       // this->send_some_viewchange_messages(this->max_faulty_replicas_allowed() + 1, is_viewchange);
    }

    TEST_F(pbft_viewchange_test, pbft_primary_sends_newview_message)
    {
        this->build_pbft();
        this->send_some_viewchange_messages(2 * this->max_faulty_replicas_allowed(), is_newview);
    }
    
    
    TEST_F(pbft_viewchange_test, backup_accepts_newview)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();


        EXPECT_TRUE(!pbft->is_primary());
        const size_t NEW_VIEW = this->pbft->get_view() + 1;

        EXPECT_EQ(this->pbft->get_view(), static_cast<size_t>(1));

        pbft_msg newview; // <NEWVIEW v+1, V -> Viewchange messages, O ->Prepare messages>
        newview.set_type(PBFT_MSG_NEWVIEW);
        newview.set_sender(TEST_NODE_UUID);
        newview.set_view(NEW_VIEW);

        // ??? newview.set_viewchange_messages( < 2f+1 V > )

        // ??? newview.set_preprepare_messages(< O >)


        this->pbft->handle_message(newview, this->default_original_msg);

        // it then moves to view v + 1
        EXPECT_EQ(this->pbft->get_view(), NEW_VIEW);

        // processing the preprepares in O as normal
        // ????
    }
}