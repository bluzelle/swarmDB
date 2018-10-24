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

using namespace ::testing;

namespace {

    const std::vector<bzn::peer_address_t> TEST_PEER_LIST{{"127.0.0.1", 8081, 8881, "name1", "uuid1"},
                                                          {"127.0.0.1", 8082, 8882, "name2", "uuid2"},
                                                          {"127.0.0.1", 8083, 8883, "name3", "uuid3"},
                                                          {"127.0.0.1", 8084, 8884, "name4", "uuid4"}};


    class pbft_config_store_test : public Test
    {
    public:
        bzn::pbft_config_store store;
    };

    TEST_F(pbft_config_store_test, initially_empty)
    {
        EXPECT_EQ(this->store.current(), nullptr);
    }

    TEST_F(pbft_config_store_test, add_test)
    {
        auto config = std::make_shared<bzn::pbft_configuration>();
        EXPECT_TRUE(this->store.add(config));

        auto config2 = this->store.get(config->get_hash());
        ASSERT_TRUE(config2 != nullptr);
        EXPECT_TRUE(*config == *config2);
    }

    TEST_F(pbft_config_store_test, current_test)
    {
        auto config = std::make_shared<bzn::pbft_configuration>();
        EXPECT_TRUE(this->store.add(config));
        EXPECT_EQ(this->store.current(), nullptr);

        auto config2 = std::make_shared<bzn::pbft_configuration>();
        config2->add_peer(TEST_PEER_LIST[0]);
        EXPECT_TRUE(this->store.add(config2));
        EXPECT_EQ(this->store.current(), nullptr);

        EXPECT_TRUE(this->store.set_current(config->get_hash()));
        EXPECT_EQ(*(this->store.current()), *config);
        EXPECT_NE(*(this->store.current()), *config2);

        EXPECT_TRUE(this->store.set_current(config2->get_hash()));
        EXPECT_EQ(*(this->store.current()), *config2);
        EXPECT_NE(*(this->store.current()), *config);
    }

    TEST_F(pbft_config_store_test, enable_test)
    {
        auto config = std::make_shared<bzn::pbft_configuration>();
        EXPECT_TRUE(this->store.add(config));
        EXPECT_FALSE(this->store.is_enabled(config->get_hash()));

        EXPECT_TRUE(this->store.enable(config->get_hash()));
        EXPECT_TRUE(this->store.is_enabled(config->get_hash()));
        EXPECT_TRUE(this->store.enable(config->get_hash(), false));
        EXPECT_FALSE(this->store.is_enabled(config->get_hash()));
    }

    TEST_F(pbft_config_store_test, removal_test)
    {
        std::vector<bzn::hash_t> hashes;
        for (auto const& p : TEST_PEER_LIST)
        {
            auto cfg = std::make_shared<bzn::pbft_configuration>();
            cfg->add_peer(p);
            hashes.push_back(cfg->get_hash());
            this->store.add(cfg);
        }

        for (auto const& h : hashes)
        {
            EXPECT_NE(this->store.get(h), nullptr);
        }

        auto config = std::make_shared<bzn::pbft_configuration>();
        EXPECT_TRUE(this->store.add(config));
        EXPECT_TRUE(this->store.remove_prior_to(config->get_hash()));

        // all other configs should be gone except the last one
        for (auto const& h : hashes)
        {
            EXPECT_EQ(this->store.get(h), nullptr);
        }
        EXPECT_NE(this->store.get(config->get_hash()), nullptr);
    }
}
