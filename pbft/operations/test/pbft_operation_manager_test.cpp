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

#include <gtest/gtest.h>
#include <bootstrap/bootstrap_peers_base.hpp>
#include <pbft/operations/pbft_operation_manager.hpp>
#include <proto/pbft.pb.h>

using namespace ::testing;

namespace
{
    const std::vector<bzn::peer_address_t> TEST_PEER_LIST{{  "127.0.0.1", 8081, 8881, "name1", "uuid0"}
                                           , {"127.0.0.1", 8082, 8882, "name2", "uuid2"}
                                           , {"127.0.0.1", 8083, 8883, "name3", "uuid3"}
                                           , {"127.0.0.1", 8084, 8884, "name4", "uuid4"}};

    const auto peers_ptr = std::make_shared<std::vector<bzn::peer_address_t>>(TEST_PEER_LIST);

    void make_prepared(const std::shared_ptr<bzn::pbft_operation>& op)
    {
        bzn_envelope outer;
        pbft_msg inner;

        inner.set_type(PBFT_MSG_PREPREPARE);
        op->record_pbft_msg(inner, outer);

        inner.set_type(PBFT_MSG_PREPARE);
        for (const auto& peer : TEST_PEER_LIST)
        {
            outer.set_sender(peer.uuid);
            op->record_pbft_msg(inner, outer);
        }

        database_msg req;
        outer.set_database_msg(req.SerializeAsString());

        op->record_request(outer);

        op->advance_operation_stage(bzn::pbft_operation_stage::commit);
        EXPECT_TRUE(op->is_prepared());
    }


    TEST(pbft_operation_manager_test, returned_operation_matches_key)
    {
        bzn::pbft_operation_manager manager;
        auto op = manager.find_or_construct(1, 6, "hash", peers_ptr);
        EXPECT_EQ(op->get_view(), 1u);
        EXPECT_EQ(op->get_sequence(), 6u);
        EXPECT_EQ(op->get_request_hash(), "hash");
    }

    TEST(pbft_operation_manager_test, returns_same_operation_instance)
    {
        bzn::pbft_operation_manager manager;
        auto op1 = manager.find_or_construct(1, 6, "hash", peers_ptr);
        auto op2 = manager.find_or_construct(2, 6, "hash", peers_ptr);
        auto op3 = manager.find_or_construct(1, 6, "hash", peers_ptr);

        EXPECT_NE(op1, op2);
        EXPECT_EQ(op1, op3);
    }

    TEST(pbft_operation_manager_test, prepared_operations_since_only_prepared_ops)
    {
        bzn::pbft_operation_manager manager;
        auto op1 = manager.find_or_construct(1, 1, "hash", peers_ptr);
        auto op2 = manager.find_or_construct(1, 2, "hash", peers_ptr);

        make_prepared(op1);

        auto prepared = manager.prepared_operations_since(0);
        EXPECT_EQ(prepared.size(), 1u);
        EXPECT_EQ(prepared.begin()->second, op1);
    }

    TEST(pbft_operation_manager_test, prepared_operations_since_prefers_prepared_ops_when_duplicate)
    {
        bzn::pbft_operation_manager manager;
        auto op1 = manager.find_or_construct(1, 1, "hash", peers_ptr);
        auto op2 = manager.find_or_construct(2, 1, "hash", peers_ptr);

        make_prepared(op2);

        auto prepared = manager.prepared_operations_since(0);
        EXPECT_EQ(prepared.begin()->second, op2);
    }

    TEST(pbft_operation_manager_test, prepared_operations_since_no_duplicates)
    {
        bzn::pbft_operation_manager manager;
        auto op1 = manager.find_or_construct(1, 1, "hash", peers_ptr);
        auto op2 = manager.find_or_construct(2, 1, "hash", peers_ptr);
        auto op3 = manager.find_or_construct(3, 1, "hash", peers_ptr);
        auto op4 = manager.find_or_construct(4, 1, "hash", peers_ptr);

        make_prepared(op1);
        make_prepared(op2);

        auto prepared = manager.prepared_operations_since(0);
        EXPECT_EQ(prepared.size(), 1u);

    }

    TEST(pbft_operation_manager_test, delete_clears_operations)
    {
        bzn::pbft_operation_manager manager;
        auto op1 = manager.find_or_construct(1, 1, "hash", peers_ptr);
        auto op2 = manager.find_or_construct(2, 2, "hash", peers_ptr);
        auto op3 = manager.find_or_construct(3, 3, "hash", peers_ptr);
        auto op4 = manager.find_or_construct(4, 4, "hash", peers_ptr);

        EXPECT_EQ(manager.held_operations_count(), 4u);
        manager.delete_operations_until(2);
        EXPECT_EQ(manager.held_operations_count(), 2u);
        manager.delete_operations_until(4);
        EXPECT_EQ(manager.held_operations_count(), 0u);

    }
}
