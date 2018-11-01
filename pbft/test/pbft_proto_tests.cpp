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

using namespace ::testing;

namespace bzn
{
    using namespace test;

    std::shared_ptr<pbft_operation>
    pbft_proto_test::send_request()
    {
        // after request is sent, SUT will send out pre-prepares to all nodes
        auto operation = std::shared_ptr<pbft_operation>();
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_preprepare, Eq(true)))).Times(
                Exactly(TEST_PEER_LIST.size()))
            .WillRepeatedly(Invoke([&](auto, auto enc_bzn_msg) {
                wrapped_bzn_msg wmsg;
                wmsg.ParseFromString(*enc_bzn_msg);

                pbft_msg msg;
                if (msg.ParseFromString(wmsg.payload()))
                {
                    if (operation == nullptr)
                        operation = this->pbft->find_operation(this->view, msg.sequence(), msg.request());

                    // the SUT needs the pre-prepare it sends to itself in order to execute state machine
                    this->send_preprepare(operation);
                }
            }));

        // sending the initial request from a client
        auto request = new pbft_request();
        request->set_type(PBFT_REQ_DATABASE);
        auto dmsg = new database_msg;
        auto create = new database_create;
        create->set_key(std::string("key_" + std::to_string(++this->index)));
        create->set_value(std::string("value_" + std::to_string(++this->index)));
        dmsg->set_allocated_create(create);
        request->set_allocated_operation(dmsg);

        bzn::json_message empty_json_msg;
        pbft->handle_request(*request, empty_json_msg);

        return operation;
    }

    // send a preprepare message to SUT
    void
    pbft_proto_test::send_preprepare(std::shared_ptr<pbft_operation> operation)
    {
        // after preprepares is sent, SUT will send out prepares to all nodes
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_prepare, Eq(true)))).Times(
                Exactly(TEST_PEER_LIST.size()));

        auto peer = *(TEST_PEER_LIST.begin());
        pbft_msg preprepare;

        preprepare.set_view(this->view);
        preprepare.set_sequence(operation->sequence);
        preprepare.set_type(PBFT_MSG_PREPREPARE);
        preprepare.set_allocated_request(new pbft_request(operation->request));
        auto wmsg = wrap_pbft_msg(preprepare);
        wmsg.set_sender(peer.uuid);
        pbft->handle_message(preprepare, wmsg);
    }

    // send fake prepares from all nodes to SUT
    void
    pbft_proto_test::send_prepares(std::shared_ptr<pbft_operation> operation)
    {
        // after prepares are sent, SUT will send out commits to all nodes
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_commit, Eq(true)))).Times(
                Exactly(TEST_PEER_LIST.size()));

        for (const auto peer : TEST_PEER_LIST)
        {
            pbft_msg prepare;

            prepare.set_view(this->view);
            prepare.set_sequence(operation->sequence);
            prepare.set_type(PBFT_MSG_PREPARE);
            prepare.set_allocated_request(new pbft_request(operation->request));
            auto wmsg = wrap_pbft_msg(prepare);
            wmsg.set_sender(peer.uuid);
            pbft->handle_message(prepare, wmsg);
        }
    }

    // send fake commits from all nodes to SUT
    void
    pbft_proto_test::send_commits(std::shared_ptr<pbft_operation> operation)
    {
        // after commits are sent, SUT will post the operation for execution
        // we want to simulate that it's been executed successfully
        EXPECT_CALL(*(this->mock_io_context), post(_)).Times(Exactly(1))
            .WillRepeatedly(Invoke([&](auto)
            {
                //this->service_execute_handler(operation->request, operation->sequence);
            }));

        for (const auto peer : TEST_PEER_LIST)
        {
            pbft_msg commit;

            commit.set_view(this->view);
            commit.set_sequence(operation->sequence);
            commit.set_type(PBFT_MSG_COMMIT);
            commit.set_allocated_request(new pbft_request(operation->request));
            auto wmsg = wrap_pbft_msg(commit);
            wmsg.set_sender(peer.uuid);
            pbft->handle_message(commit, wmsg);
        }

        // tell pbft that this operation has been executed
        this->service_execute_handler(operation->request, operation->sequence);
    }

    void
    pbft_proto_test::prepare_for_checkpoint(size_t seq)
    {
        // pbft needs a hash for this checkpoint
        EXPECT_CALL(*this->mock_service, service_state_hash(seq)).Times(Exactly(1))
            .WillRepeatedly(Invoke([&](auto)
            {
                return std::to_string(seq);
            }));

        // after enough commits are sent, SUT will send out checkpoint message to all nodes
        EXPECT_CALL(*this->mock_node, send_message_str(_, ResultOf(test::is_checkpoint, Eq(true)))).Times(
            Exactly(TEST_PEER_LIST.size()));

    }

    void
    pbft_proto_test::force_checkpoint(size_t seq)
    {
        prepare_for_checkpoint(seq);
        this->pbft->checkpoint_reached_locally(seq);
    }

    void
    pbft_proto_test::run_transaction_through_primary(bool commit)
    {
        // send request to SUT and handle expected calls
        auto op = send_request();
        ASSERT_NE(op, nullptr);

        // send node prepares to SUT
        send_prepares(op);

        // send node commits to SUT
        if (commit)
        {
            send_commits(op);
        }
    }

    void
    pbft_proto_test::run_transaction_through_backup(bool /*commit*/)
    {
        // send pre-prepare to SUT

        // send prepares to SUT

        // send commits to SUT
    }

    TEST_F(pbft_proto_test, init)
    {
        this->build_pbft();

#if 1
        for (size_t i = 0; i < 99; i++)
            run_transaction_through_primary();
        prepare_for_checkpoint(100);
        run_transaction_through_primary();
#else
        for (size_t i = 0; i < 9; i++)
            run_transaction_through_primary();
        force_checkpoint(10);
        run_transaction_through_primary();
#endif
    }
}

