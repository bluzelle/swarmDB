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
        std::shared_ptr<pbft_operation> send_request();

        // send a preprepare message to SUT
        void send_preprepare(uint64_t sequence, const pbft_request& request);

        // send fake prepares from all nodes to SUT
        void send_prepares(uint64_t sequence, const pbft_request& request);

        // send fake commits from all nodes to SUT
        void send_commits(uint64_t sequence, const pbft_request& request);

        // set expectations for upcoming checkpoint
        void prepare_for_checkpoint(size_t seq);

        // send a checkpoint message on behalf of a node
        void send_checkpoint(bzn::peer_address_t node, uint64_t sequence);

        // send checkpoints from all nodes
        void stabilize_checkpoint(size_t seq);

        // get SUT to create a checkpoint at a specific sequence number
        void force_checkpoint(size_t seq);

        // send a database request through a primary all the way to possibly committing it
        void run_transaction_through_primary(bool commit = true);

        // send a database request through a backup node all the way to possibly committing it
        void run_transaction_through_backup(bool commit = true);

        // return the number of nodes that represent "f"
        size_t faulty_nodes_bound() const;

        // current request sequence
        size_t index = 0;

        // current view
        uint64_t view = 1;
    };
}

