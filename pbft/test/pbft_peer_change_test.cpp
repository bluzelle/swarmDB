// Copyright (C) 2019 Bluzelle
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

#include <pbft/test/pbft_test_common.hpp>

/*
 * the goal here is to check that pbft remains functional and does the right thing even when the peers list
 * can change at any moment, by putting it through a series of tests and each time changing the peers list at a different moment
 */


namespace
{
    const bzn::peers_list_t PEER_LIST_A{{  "127.0.0.1", 8080, "name0", "uuid0"}
                                           , {"127.0.0.1", 8081, "name1", "uuid1"}
                                           , {"127.0.0.1", 8082, "name2", "uuid2"}
                                           , {"127.0.0.1", 8083, "name3", "uuid3"}};

    const bzn::peers_list_t PEER_LIST_A_PLUS{{  "127.0.0.1", 8080, "name0", "uuid0"}
                                        , {"127.0.0.1", 8081, "name1", "uuid1"}
                                        , {"127.0.0.1", 8082, "name2", "uuid2"}
                                        , {"127.0.0.1", 8083, "name3", "uuid3"}
                                        , {"127.0.0.1", 8084, "name4", "uuid4"}};

    const bzn::peers_list_t PEER_LIST_B{{  "127.0.0.1", 8080, "name0", "uuid0"}
                                        , {"127.0.0.1", 8081, "name1", "uuid1"}
                                        , {"127.0.0.1", 8082, "name2", "uuid2"}
                                        , {"127.0.0.1", 8085, "name5", "uuid5"}};

    using peer_switch_t = std::pair<bzn::peers_list_t, bzn::peers_list_t>;
    const peer_switch_t add_peer = std::make_pair(PEER_LIST_A, PEER_LIST_A_PLUS);
    const peer_switch_t remove_peer = std::make_pair(PEER_LIST_A_PLUS, PEER_LIST_A);
    const peer_switch_t replace_peer = std::make_pair(PEER_LIST_A, PEER_LIST_B);

    // which switch point are we using, how many are there total, before peers list, after peers list
    // the second int is passed through so that the test can verify that the callsite has the correct (updated)
    // total number of switch points
    using test_param_t = std::tuple<unsigned int, unsigned int, peer_switch_t>;
}

using namespace ::testing;

class changeover_test : public bzn::test::pbft_test, public testing::WithParamInterface<test_param_t>
{

public:

    changeover_test()
        : current_peers_list(std::make_shared<bzn::peers_list_t>(this->before_list()))
    {
        this->set_up_beacon();
    }

    ~changeover_test()
    {
        EXPECT_EQ(this->potential_switch_points_hit, this->max_change_point());
    }

protected:
    void switch_here()
    {
        if (this->potential_switch_points_hit == this->this_change_point())
        {
            this->do_switch();
        }

        this->potential_switch_points_hit++;
    }

    unsigned int this_change_point()
    {
        return std::get<0>(GetParam());
    }

    unsigned int max_change_point()
    {
        return std::get<1>(GetParam());
    }

    bzn::peers_list_t before_list()
    {
        auto pair = std::get<2>(GetParam());
        return pair.first;
    }

    bzn::peers_list_t after_list()
    {
        auto pair = std::get<2>(GetParam());
        return pair.second;
    }

private:
    void do_switch()
    {
        this->current_peers_list = std::make_shared<bzn::peers_list_t>(this->after_list());
    }

    void set_up_beacon()
    {
        auto changing_beacon = std::make_shared<bzn::mock_peers_beacon_base>();

        EXPECT_CALL(*changing_beacon, start()).Times(AtMost(1));

        EXPECT_CALL(*changing_beacon, current()).WillRepeatedly(Invoke(
                [&]()
                {
                    return this->current_peers_list;
                }));

        EXPECT_CALL(*changing_beacon, ordered()).WillRepeatedly(Invoke(
                [&]()
                {
                    auto ordered = std::make_shared<bzn::ordered_peers_list_t>();
                    std::for_each(this->current_peers_list->begin(), this->current_peers_list->end(),
                            [&](const auto& peer)
                            {
                                ordered->push_back(peer);
                            });

                    std::sort(ordered->begin(), ordered->end(),
                            [](const auto& peer1, const auto& peer2)
                            {
                                return peer1.uuid.compare(peer2.uuid) < 0;
                            });

                    return ordered;
                }));

        EXPECT_CALL(*changing_beacon, refresh(_)).Times(AnyNumber());
        this->beacon = changing_beacon;
    }

    std::shared_ptr<bzn::peers_list_t> current_peers_list;

    unsigned int potential_switch_points_hit = 0;
};

TEST_P(changeover_test, perform_pbft_operations)
{
    switch_here();
    EXPECT_FALSE(true);
}

TEST_P(changeover_test, perform_checkpoinnt)
{
    switch_here();
    EXPECT_FALSE(true);
}

TEST_P(changeover_test, perform_viewchange)
{
    switch_here();
    EXPECT_FALSE(true);
}

// going to have to either
// - run some redundant tests
// - create a test suite for each test

INSTANTIATE_TEST_CASE_P(changeover_test_set, changeover_test,
        testing::Combine(
                Range(0u, 1u),
                Values(1u),
                Values(add_peer, remove_peer, replace_peer)
        ),);

