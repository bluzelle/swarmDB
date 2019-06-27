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

namespace
{
    uint64_t MAX_REQUEST_SIZE{249 * 1024};
//    uint64_t MAX_REQUEST_SIZE{10 * 1024};
}

namespace bzn
{
    using namespace test;

    size_t
    pbft_proto_test::faulty_nodes_bound() const
    {
        return this->pbft->max_faulty_nodes();
    }

    std::shared_ptr<pbft_operation>
    pbft_proto_test::send_request()
    {
        // after request is sent, SUT will send out pre-prepares to all nodes
        auto operation = std::shared_ptr<pbft_operation>();
        EXPECT_CALL(*this->mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_preprepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()))
            .WillRepeatedly(Invoke([&](auto, auto wmsg)
            {
                pbft_msg msg;
                if (msg.ParseFromString(wmsg->pbft()))
                {
                    if (operation == nullptr)
                    {
                        operation = this->operation_manager->find_or_construct(this->view, msg.sequence(), msg.request_hash(), this->pbft->current_peers_ptr());

                        // the SUT needs the pre-prepare it sends to itself in order to execute state machine
                        this->send_preprepare(operation->get_sequence(), operation->get_request());
                    }
                }
            }));

        bzn_envelope request;
        database_msg dmsg;
        dmsg.mutable_create()->set_key(std::string("key_" + std::to_string(++this->index)));

        dmsg.mutable_create()->set_value(std::string(MAX_REQUEST_SIZE, 'a'));

        request.set_database_msg(dmsg.SerializeAsString());
        request.set_timestamp(this->now());
        request.set_sender(this->pbft->get_uuid());

        pbft->handle_request(request);

        return operation;
    }

    // send a preprepare message to SUT
    void
    pbft_proto_test::send_preprepare(uint64_t sequence, const bzn_envelope& request)
    {
        // after preprepare is sent, SUT will send out prepares to all nodes
        EXPECT_CALL(*this->mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_prepare, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));

        auto peer = *(TEST_PEER_LIST.begin());
        pbft_msg preprepare;

        preprepare.set_view(this->view);
        preprepare.set_sequence(sequence);
        preprepare.set_type(PBFT_MSG_PREPREPARE);

        if (request.payload_case() == bzn_envelope::kPbftInternalRequest)
        {
            preprepare.set_allocated_request(new bzn_envelope(request));
        }

        preprepare.set_request_hash(this->pbft->crypto->hash(request));
        auto wmsg = wrap_pbft_msg(preprepare, peer.uuid);
        if (request.payload_case() != bzn_envelope::kPbftInternalRequest)
        {
            *wmsg.add_piggybacked_requests() = request;
        }
        pbft->handle_message(preprepare, wmsg);
    }

    // send fake prepares from all nodes to SUT
    void
    pbft_proto_test::send_prepares(uint64_t sequence, const bzn::hash_t& request_hash)
    {
        // after prepares are sent, SUT will send out commits to all nodes
        EXPECT_CALL(*this->mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_commit, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));

        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg prepare;

            prepare.set_view(this->view);
            prepare.set_sequence(sequence);
            prepare.set_type(PBFT_MSG_PREPARE);
            prepare.set_request_hash(request_hash);
            auto wmsg = wrap_pbft_msg(prepare, peer.uuid);
            pbft->handle_message(prepare, wmsg);
        }
    }

    // send fake commits from all nodes to SUT
    void
    pbft_proto_test::send_commits(uint64_t sequence, const bzn::hash_t& request_hash)
    {
        // after commits are sent, SUT will post the operation for execution
        // we want to simulate that it's been executed successfully
        EXPECT_CALL(*(this->mock_io_context), post(_)).Times(Exactly(1));

        for (const auto& peer : TEST_PEER_LIST)
        {
            pbft_msg commit;

            commit.set_view(this->view);
            commit.set_sequence(sequence);
            commit.set_type(PBFT_MSG_COMMIT);
            commit.set_request_hash(request_hash);
            auto wmsg = wrap_pbft_msg(commit, peer.uuid);
            pbft->handle_message(commit, wmsg);
        }

        // tell pbft that this operation has been executed
        this->service_execute_handler(this->operation_manager->find_or_construct(this->view, sequence, request_hash, pbft->current_peers_ptr()));
    }

    void
    pbft_proto_test::prepare_for_checkpoint(size_t seq)
    {
        // pbft needs a hash for this checkpoint
        EXPECT_CALL(*this->mock_service, service_state_hash(seq)).Times(AnyNumber())
            .WillRepeatedly(Invoke([&](auto s)
            {
                return std::to_string(s);
            }));

        // after enough commits are sent, SUT will send out checkpoint message to all nodes
        EXPECT_CALL(*this->mock_node, send_signed_message(A<const boost::asio::ip::tcp::endpoint&>(), ResultOf(test::is_checkpoint, Eq(true))))
            .Times(Exactly(TEST_PEER_LIST.size()));
    }

    void
    pbft_proto_test::force_checkpoint(size_t seq)
    {
        this->pbft->checkpoint_manager->local_checkpoint_reached(checkpoint_t{seq, "dummy checkpoint state"});
    }

    checkpoint_msg
    pbft_proto_test::build_checkpoint_msg(uint64_t sequence)
    {
        checkpoint_msg cp;
        cp.set_sequence(sequence);
        cp.set_state_hash(std::to_string(sequence));

        return cp;
    }

    void
    pbft_proto_test::send_checkpoint(bzn::peer_address_t node, uint64_t sequence)
    {
        auto msg = build_checkpoint_msg(sequence);
        bzn_envelope wrapper;
        wrapper.set_checkpoint_msg(msg.SerializeAsString());
        wrapper.set_sender(node.uuid);
        this->pbft->checkpoint_manager->handle_checkpoint_message(wrapper);
    }

    void
    pbft_proto_test::stabilize_checkpoint(size_t seq)
    {
        for (const auto& peer : TEST_PEER_LIST)
        {
            if (peer.uuid == this->uuid)
            {
                continue;
            }

            this->send_checkpoint(peer, seq);
        }
    }

    void
    pbft_proto_test::run_transaction_through_primary(bool commit)
    {
        // send request to SUT and handle expected calls
        auto op = send_request();
        ASSERT_NE(op, nullptr);

        // send node prepares to SUT
        send_prepares(op->get_sequence(), op->get_request_hash());

        // send node commits to SUT
        if (commit)
        {
            send_commits(op->get_sequence(), op->get_request_hash());
        }
    }

    void
    pbft_proto_test::run_transaction_through_backup(bool commit)
    {
        // create request
        bzn_envelope request;
        database_msg dmsg;
        dmsg.mutable_create()->set_key(std::string("key_" + std::to_string(++this->index)));
        dmsg.mutable_create()->set_value(std::string("value_" + std::to_string(this->index)));
        request.set_database_msg(dmsg.SerializeAsString());

        // send pre-prepare to SUT
        send_preprepare(this->index, request);

        // send prepares to SUT
        auto request_hash = this->pbft->crypto->hash(request);
        send_prepares(this->index, request_hash);

        // send commits to SUT
        if (commit)
        {
            send_commits(this->index, request_hash);
        }
    }

    TEST_F(pbft_proto_test, test_primary_full_checkpoint)
    {
        this->build_pbft();

        for (size_t i = 0; i < 99; i++)
        {
            run_transaction_through_primary();
        }
        prepare_for_checkpoint(100);
        run_transaction_through_primary();
    }

    TEST_F(pbft_proto_test, test_primary_quick_checkpoint)
    {
        this->build_pbft();

        for (size_t i = 0; i < 9; i++)
        {
            run_transaction_through_primary();
        }
        prepare_for_checkpoint(10);
        run_transaction_through_primary();
        force_checkpoint(10);
    }

    TEST_F(pbft_proto_test, test_backup_full_checkpoint)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        for (size_t i = 0; i < 99; i++)
        {
            run_transaction_through_backup();
        }
        prepare_for_checkpoint(100);
        run_transaction_through_backup();
    }

    TEST_F(pbft_proto_test, test_backup_quick_checkpoint)
    {
        this->uuid = SECOND_NODE_UUID;
        this->build_pbft();

        for (size_t i = 0; i < 9; i++)
        {
            run_transaction_through_backup();
        }
        prepare_for_checkpoint(10);
        run_transaction_through_backup();
        force_checkpoint(10);
    }
}

