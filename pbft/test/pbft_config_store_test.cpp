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

#include <gtest/gtest.h>
#include <include/bluzelle.hpp>
#include <pbft/pbft_config_store.hpp>
#include <storage/mem_storage.hpp>
#include <pbft/pbft.hpp>

using namespace ::testing;

namespace bzn
{

    const std::vector<bzn::peer_address_t> TEST_PEER_LIST{{"127.0.0.1", 8081, "name1", "uuid1"},
                                                          {"127.0.0.1", 8082, "name2", "uuid2"},
                                                          {"127.0.0.1", 8083, "name3", "uuid3"},
                                                          {"127.0.0.1", 8084, "name4", "uuid4"}};


    class pbft_config_store_test : public Test
    {
    public:
        pbft_config_store_test()
        : storage(std::make_shared<bzn::mem_storage>())
        , store(storage)
        {}

        std::shared_ptr<bzn::storage_base> storage;
        bzn::pbft_config_store store;
    };

    TEST_F(pbft_config_store_test, initially_empty)
    {
        EXPECT_EQ(this->store.current(), nullptr);
    }

    TEST_F(pbft_config_store_test, add_test)
    {
        auto config = std::make_shared<bzn::pbft_configuration>();
        this->store.add(config);

        auto config2 = this->store.get(config->get_hash());
        ASSERT_TRUE(config2 != nullptr);
        EXPECT_TRUE(*config == *config2);
    }

    TEST_F(pbft_config_store_test, current_test)
    {
        auto config = std::make_shared<bzn::pbft_configuration>();
        this->store.add(config);
        EXPECT_EQ(this->store.current(), nullptr);

        auto config2 = std::make_shared<bzn::pbft_configuration>();
        config2->add_peer(TEST_PEER_LIST[0]);
        this->store.add(config2);
        EXPECT_EQ(this->store.current(), nullptr);

        EXPECT_TRUE(this->store.set_current(config->get_hash(), 1));
        EXPECT_EQ(*(this->store.current()), *config);
        EXPECT_NE(*(this->store.current()), *config2);

        EXPECT_TRUE(this->store.set_current(config2->get_hash(), 2));
        EXPECT_EQ(*(this->store.current()), *config2);
        EXPECT_NE(*(this->store.current()), *config);
    }

    TEST_F(pbft_config_store_test, state_test)
    {
        auto config = std::make_shared<bzn::pbft_configuration>();
        this->store.add(config);
        EXPECT_EQ(this->store.get_state(config->get_hash()), pbft_config_store::pbft_config_state::accepted);
        EXPECT_TRUE(this->store.set_prepared(config->get_hash()));
        EXPECT_EQ(this->store.get_state(config->get_hash()), pbft_config_store::pbft_config_state::prepared);
        EXPECT_TRUE(this->store.set_committed(config->get_hash()));
        EXPECT_EQ(this->store.get_state(config->get_hash()), pbft_config_store::pbft_config_state::committed);
        EXPECT_TRUE(this->store.set_current(config->get_hash(), 1));
        EXPECT_EQ(this->store.get_state(config->get_hash()), pbft_config_store::pbft_config_state::current);
        EXPECT_TRUE(this->store.set_current(config->get_hash(), 2));

        auto config2 = std::make_shared<bzn::pbft_configuration>();
        config2->add_peer(TEST_PEER_LIST[0]);
        EXPECT_EQ(this->store.get_state(config2->get_hash()), pbft_config_store::pbft_config_state::unknown);
        EXPECT_FALSE(this->store.set_prepared(config2->get_hash()));
        EXPECT_FALSE(this->store.set_committed(config2->get_hash()));
        EXPECT_FALSE(this->store.set_current(config2->get_hash(), 3));

        this->store.add(config2);
        EXPECT_FALSE(this->store.set_current(config2->get_hash(), 2));
        EXPECT_TRUE(this->store.set_current(config2->get_hash(), 3));
        EXPECT_EQ(this->store.get_state(config2->get_hash()), pbft_config_store::pbft_config_state::current);
        EXPECT_EQ(this->store.current(), config2);
        EXPECT_EQ(this->store.get_state(config->get_hash()), pbft_config_store::pbft_config_state::current);
        EXPECT_NE(this->store.current(), config);

        EXPECT_EQ(this->store.get(1)->get_hash(), config->get_hash());
        EXPECT_EQ(this->store.get(2)->get_hash(), config->get_hash());
        EXPECT_EQ(this->store.get(3)->get_hash(), config2->get_hash());
    }

    TEST_F(pbft_config_store_test, newest_test)
    {
        bzn::hash_t empty_hash;

        auto config1 = std::make_shared<bzn::pbft_configuration>();
        this->store.add(config1);
        auto config2 = std::make_shared<bzn::pbft_configuration>();
        config2->add_peer(TEST_PEER_LIST[0]);
        this->store.add(config2);
        auto config3 = std::make_shared<bzn::pbft_configuration>(*config2);
        config3->add_peer(TEST_PEER_LIST[1]);
        this->store.add(config3);

        EXPECT_EQ(this->store.newest_prepared(), empty_hash);
        EXPECT_EQ(this->store.newest_committed(), empty_hash);
        EXPECT_EQ(this->store.current(), nullptr);

        EXPECT_TRUE(this->store.set_prepared(config1->get_hash()));
        EXPECT_EQ(this->store.newest_prepared(), config1->get_hash());

        EXPECT_TRUE(this->store.set_committed(config2->get_hash()));
        EXPECT_EQ(this->store.newest_prepared(), config2->get_hash());
        EXPECT_EQ(this->store.newest_committed(), config2->get_hash());

        EXPECT_TRUE(this->store.set_prepared(config3->get_hash()));
        EXPECT_TRUE(this->store.set_current(config3->get_hash(), 1));
        EXPECT_EQ(this->store.newest_prepared(), config3->get_hash());
        EXPECT_EQ(this->store.newest_committed(), config3->get_hash());

        EXPECT_TRUE(this->store.set_current(config2->get_hash(), 2));
        EXPECT_EQ(this->store.newest_prepared(), config3->get_hash());
        EXPECT_EQ(this->store.newest_committed(), config3->get_hash());
        EXPECT_EQ(this->store.current(), config2);
    }

    TEST_F(pbft_config_store_test, old_becomes_new_test)
    {
        auto config1 = std::make_shared<bzn::pbft_configuration>();
        this->store.add(config1);
        auto config2 = std::make_shared<bzn::pbft_configuration>();
        config2->add_peer(TEST_PEER_LIST[0]);
        this->store.add(config2);

        EXPECT_TRUE(this->store.set_prepared(config1->get_hash()));
        EXPECT_EQ(this->store.newest_prepared(), config1->get_hash());

        EXPECT_TRUE(this->store.set_prepared(config2->get_hash()));
        EXPECT_EQ(this->store.newest_prepared(), config2->get_hash());
        this->store.add(config1);
        EXPECT_EQ(this->store.newest_prepared(), config2->get_hash());

        EXPECT_TRUE(this->store.set_prepared(config1->get_hash()));
        EXPECT_EQ(this->store.newest_prepared(), config1->get_hash());
    }
}
