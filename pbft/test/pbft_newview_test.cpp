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
#include <pbft/test/pbft_proto_test.hpp>

namespace bzn
{

    class pbft_newview_test : public pbft_proto_test
    {
    public:

        std::shared_ptr<mock_crypto_base>
        build_pft_with_mock_crypto()
        {
            std::shared_ptr<mock_crypto_base> mockcrypto = std::make_shared<mock_crypto_base>();
            this->crypto = mockcrypto;
            this->build_pbft();
            return mockcrypto;
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

            EXPECT_CALL(*mockcrypto, sign(_)).WillRepeatedly(Return(true));
            EXPECT_CALL(*mockcrypto, verify(_)).WillRepeatedly(Return(true));

            for (current_sequence=1; current_sequence < 100; ++current_sequence)
            {
                run_transaction_through_primary();
            }
            prepare_for_checkpoint(current_sequence);
            run_transaction_through_primary();
            this->stabilize_checkpoint(current_sequence);
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

        size_t
        max_faulty_replicas_allowed() { return TEST_PEER_LIST.size() / 3; }


    };

    TEST_F(pbft_newview_test, make_newview)
    {
        uint64_t current_sequence{0};
        this->generate_checkpoint_at_sequence_100(current_sequence);

        this->run_transaction_through_primary_times(2, current_sequence);

        bzn_envelope viewchange_envelope;
        EXPECT_CALL(*mock_node, send_maybe_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_viewchange, Eq(true))))
                .WillRepeatedly(Invoke([&](const auto & /*endpoint*/, const auto& viewchange_env) {viewchange_envelope = *viewchange_env;}));
        this->pbft->handle_failure();

        pbft_msg viewchange;

        viewchange.ParseFromString(viewchange_envelope.pbft());

        uint64_t                            new_view_index{viewchange.view()};
        std::map<uuid_t,bzn_envelope>       viewchange_envelopes_from_senders;
        std::map<uint64_t, bzn_envelope>    pre_prepare_messages;


        // we can generate valid newview now

        pbft_msg newview{this->pbft->make_newview(new_view_index, viewchange_envelopes_from_senders, pre_prepare_messages)};

        EXPECT_EQ(PBFT_MSG_NEWVIEW, newview.type());
        EXPECT_EQ(new_view_index, newview.view());
        EXPECT_EQ(this->pbft->next_issued_sequence_number.value(), current_sequence + 1);
    }

    TEST_F(pbft_newview_test, test_get_primary)
    {
        build_pbft();

        // the pbft sut must be the current view's primay
        EXPECT_EQ(this->uuid, this->pbft->get_current_primary().value().uuid);

        EXPECT_NE(this->uuid, this->pbft->predict_primary(this->pbft->view.value() + 1).value().uuid);

        // given a view, get_primary must provide the address of a primary

        // TODO: this is a pretty sketchy test.
        for (size_t view{0}; view < 100; ++view)
        {
            const bzn::uuid_t uuid = this->pbft->predict_primary(view).value().uuid;
            const bzn::uuid_t accepted_uuid = this->pbft->peers()->ordered()->at(view % this->pbft->peers()->ordered()->size()).uuid;
            EXPECT_EQ(uuid, accepted_uuid);
        }
    }
}
