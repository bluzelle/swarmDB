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
#include <pbft/pbft_configuration.hpp>

using namespace ::testing;

namespace
{

    const std::vector<bzn::peer_address_t> TEST_PEER_LIST{{"127.0.0.1", 8081, 8881, "name1", "uuid1"},
                                                          {"127.0.0.1", 8082, 8882, "name2", "uuid2"},
                                                          {"127.0.0.1", 8083, 8883, "name3", "uuid3"},
                                                          {"127.0.0.1", 8084, 8884, "name4", "uuid4"}};


    class pbft_configuration_test : public Test
    {
    public:
        bzn::pbft_configuration cfg;
    };


    TEST_F(pbft_configuration_test, initially_empty)
    {
        EXPECT_EQ(this->cfg.get_peers()->size(), 0U);
    }

    TEST_F(pbft_configuration_test, single_add_succeeds)
    {
        EXPECT_TRUE(this->cfg.add_peer(TEST_PEER_LIST[0]));
        EXPECT_EQ(this->cfg.get_peers()->size(), 1U);
    }

    TEST_F(pbft_configuration_test, remove_succeeds)
    {
        EXPECT_TRUE(this->cfg.add_peer(TEST_PEER_LIST[0]));
        EXPECT_EQ(this->cfg.get_peers()->size(), 1U);
        EXPECT_TRUE(this->cfg.remove_peer(TEST_PEER_LIST[0]));
        EXPECT_EQ(this->cfg.get_peers()->size(), 0U);
    }

    TEST_F(pbft_configuration_test, duplicate_add_fails)
    {
        EXPECT_TRUE(this->cfg.add_peer(TEST_PEER_LIST[0]));
        EXPECT_EQ(this->cfg.get_peers()->size(), 1U);
        EXPECT_FALSE(this->cfg.add_peer(TEST_PEER_LIST[0]));
        EXPECT_EQ(this->cfg.get_peers()->size(), 1U);
    }

    TEST_F(pbft_configuration_test, bad_remove_fails)
    {
        EXPECT_TRUE(this->cfg.add_peer(TEST_PEER_LIST[0]));
        EXPECT_EQ(this->cfg.get_peers()->size(), 1U);
        EXPECT_FALSE(this->cfg.remove_peer(TEST_PEER_LIST[1]));
        EXPECT_EQ(this->cfg.get_peers()->size(), 1U);
    }

    void check_equal(const std::vector<bzn::peer_address_t> &p1, const std::vector<bzn::peer_address_t> &p2)
    {
        ASSERT_EQ(p1.size(), p2.size());
        for (uint i = 0; i < p1.size(); i++)
        {
            EXPECT_EQ(p1[i], p2[i]);
        }
    }

    TEST_F(pbft_configuration_test, sort_order)
    {
        // add the peers in reverse order (assumes TEST_PEER_LIST is sorted)
        for (auto it = TEST_PEER_LIST.rbegin(); it != TEST_PEER_LIST.rend(); it++)
        {
            EXPECT_TRUE(this->cfg.add_peer(*it));
        }

        auto peers = this->cfg.get_peers();
        check_equal(*peers, TEST_PEER_LIST);
    }

    TEST_F(pbft_configuration_test, to_from_json)
    {
        for (const auto &peer : TEST_PEER_LIST)
        {
            this->cfg.add_peer(peer);
        }

        bzn::json_message json = this->cfg.to_json();
        bzn::pbft_configuration cfg2;
        cfg2.from_json(json);
        check_equal(*(cfg2.get_peers()), TEST_PEER_LIST);
    }

    TEST_F(pbft_configuration_test, reject_invalid_peer)
    {
        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("", 8081, 8881, "name1", "uuid1")));
        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 0, 8881, "name1", "uuid1")));
        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8081, 0, "name1", "uuid1")));
        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8081, 8881, "", "uuid1")));
        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8081, 8881, "name1", "")));
        EXPECT_TRUE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8081, 8881, "name1", "uuid1")));

        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8082, 8882, "name2", "uuid1")));
        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8082, 8882, "name1", "uuid2")));
        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8082, 8881, "name2", "uuid2")));
        EXPECT_FALSE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8081, 8882, "name2", "uuid2")));
        EXPECT_TRUE(this->cfg.add_peer(bzn::peer_address_t("127.0.0.1", 8082, 8882, "name2", "uuid2")));
    }
}
