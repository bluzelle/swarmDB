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
    TEST_F(pbft_test, test_local_commit_sends_audit_messages)
    {
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_audit, Eq(false))))
                .Times(AnyNumber());
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_audit, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->build_pbft();
        this->pbft->set_audit_enabled(true);

        wrapped_bzn_msg dummy_original_msg;
        pbft_msg preprepare = pbft_msg(this->preprepare_msg);
        preprepare.set_sequence(1);
        this->pbft->handle_message(preprepare, dummy_original_msg);

        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg prepare = pbft_msg(preprepare);
            wrapped_bzn_msg prepare_wrap;
            pbft_msg commit = pbft_msg(preprepare);
            wrapped_bzn_msg commit_wrap;
            prepare.set_type(PBFT_MSG_PREPARE);
            prepare_wrap.set_sender(peer.uuid);
            commit.set_type(PBFT_MSG_COMMIT);
            commit_wrap.set_sender(peer.uuid);
            this->pbft->handle_message(prepare, prepare_wrap);
            this->pbft->handle_message(commit, commit_wrap);
        }
    }

    TEST_F(pbft_test, primary_sends_primary_status)
    {
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_audit, Eq(true))))
                .Times(Exactly(TEST_PEER_LIST.size()));

        this->build_pbft();
        this->pbft->set_audit_enabled(true);
        ASSERT_TRUE(this->pbft->is_primary());

        this->audit_heartbeat_timer_callback(boost::system::error_code());
    }

    TEST_F(pbft_test, nonprimary_does_not_send_primary_status)
    {
        EXPECT_CALL(*mock_node, send_message_str(_, ResultOf(is_audit, Eq(true))))
                .Times(Exactly(0));

        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();
        this->pbft->set_audit_enabled(true);
        ASSERT_FALSE(this->pbft->is_primary());

        this->audit_heartbeat_timer_callback(boost::system::error_code());
    }
}

