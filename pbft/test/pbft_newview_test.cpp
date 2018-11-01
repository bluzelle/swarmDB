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
        send_f_plus_one_viewchange_messages()
        {
            size_t count{0};
            const uint64_t NON_FAULTY_REPLICAS = TEST_PEER_LIST.size() / 3;

            EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_viewchange, Eq(true))))
                    .Times(Exactly(TEST_PEER_LIST.size()))
                    .WillRepeatedly(Invoke([&](auto &, auto &) { EXPECT_EQ((NON_FAULTY_REPLICAS + 1), count); }));


            const size_t NEW_VIEW = this->pbft->get_view() + 1;
            pbft_msg pbft_msg;
            pbft_msg.set_type(PBFT_MSG_VIEWCHANGE);
            pbft_msg.set_view(NEW_VIEW);

            // let's pretend that the sytem under test is receiving view change messages
            // from the other replicas
            for (const auto &peer : TEST_PEER_LIST)
            {
                count++;
                pbft_msg.set_sender(peer.uuid);
                this->pbft->handle_message(pbft_msg, this->default_original_msg);
                LOG(debug) << "\t***this->pbft->get_view(): " << this->pbft->get_view();
                if (count == (NON_FAULTY_REPLICAS + 1))
                    break;
            }


        }


    };



    TEST_F(pbft_viewchange_test, pbft_replica_sends_viewchange_message)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();
        this->send_f_plus_one_viewchange_messages();
    }



}