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

using namespace ::testing;

namespace bzn
{
    using namespace test;

    class pbft_proto_test : public pbft_test
    {
    public:
        // send a fake request to SUT
        std::shared_ptr<pbft_operation> send_request()
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
            database_msg dmsg;
            auto create = new database_create;
            create->set_key(std::string("key_" + std::to_string(++this->index)));
            create->set_value(std::string("value_" + std::to_string(++this->index)));
            dmsg.set_allocated_create(create);

            bzn::json_message empty_json_msg;
            pbft->handle_request(*request, empty_json_msg);

            return operation;
        }

        // send a preprepare message to SUT
        void send_preprepare(std::shared_ptr<pbft_operation> operation)
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
        void send_prepares(std::shared_ptr<pbft_operation> operation)
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
        void send_commits(std::shared_ptr<pbft_operation> operation)
        {
            for (const auto peer : TEST_PEER_LIST)
            {
                pbft_msg commit;

                commit.set_view(this->view);
                commit.set_sequence(operation->sequence);
                commit.set_type(PBFT_MSG_PREPARE);
                commit.set_allocated_request(new pbft_request(operation->request));
                auto wmsg = wrap_pbft_msg(commit);
                wmsg.set_sender(peer.uuid);
                pbft->handle_message(commit, wmsg);
            }
        }

        void run_transaction_through_primary()
        {
            // send request to SUT and handle expected calls
            auto op = send_request();
            ASSERT_NE(op, nullptr);

            // send node prepares to SUT
            send_prepares(op);

            // send node commits to SUT
            send_commits(op);

            // catch execution of request?
        }

        void run_transaction_through_backup()
        {
            // send pre-prepare to SUT

            // send prepares to SUT

            // send commits to SUT

            // catch execution of request?
        }

    private:
        size_t index = 0;
        uint64_t view = 1;
    };

    TEST_F(pbft_proto_test, init)
    {
        this->build_pbft();
        run_transaction_through_primary();
    }
}

