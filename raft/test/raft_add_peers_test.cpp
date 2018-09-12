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

#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_node_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <raft/raft.hpp>
#include <raft/log_entry.hpp>
#include <raft/raft_log.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <vector>
#include <random>
#include <stdlib.h>

using namespace ::testing;

namespace
{
    const std::string TEST_STATE_DIR = "./.raft_test_state/";
    const bzn::uuid_t LEADER_NODE_UUID      {"fb300a30-49fd-4230-8044-0e3069948e42"};
    const bzn::uuid_t TEST_NODE_UUID        {"f0645cc2-476b-485d-b589-217be3ca87d5"};
    const bzn::uuid_t FOLLOWER_NODE_UUID    {"8993098f-e32e-4b6f-9db9-c770c9bc2509"};
    const bzn::uuid_t CANDIDATE_NODE_UUID   {"ec5a510f-24df-4026-9739-dd3744fdec6a"};

    const bzn::peers_list_t FULL_TEST_PEER_LIST {
                                            {"127.0.0.1", 8081, 80, "leader",   LEADER_NODE_UUID},
                                            {"127.0.0.1", 8082, 81, "follower", FOLLOWER_NODE_UUID},
                                            {"127.0.0.1", 8084, 82, "sut",      TEST_NODE_UUID}};

    const bzn::peers_list_t PARTIAL_TEST_PEER_LIST{
                                            {"127.0.0.1", 8081, 80, "leader",   LEADER_NODE_UUID},
                                            {"127.0.0.1", 8082, 81, "follower", FOLLOWER_NODE_UUID}};

    const std::string signature {
            "Oo8ZlDQcMlZF4hqnhN/2Dz3FYarZHrGf+87i+JUSxBu2GKFk8SYcDrwjc0DuhUCx"
            "pRVQppMk5fjZtJ3r6I9066jcEpJPljU1SC1Thpy+AUEYx0r640SKRwKwmJMe6mRd"
            "SJ75rcYHu5+etajOWWjMs4vYQtcwfVF3oEd9/pZjea8x6PuhnM50+FPpnvgu57L8"
            "vHdeWjCqAiPyomQSLgIJPjvMJw4aHUUE3tHX1WOB8XDHdvuhi9gZODzZWbdI92JN"
            "hoLbwvjmhKTeTN+FbBtdJIjC0+V0sMFmGNJQ8WIkJscN0hzRkmdlU965lHe4hqlc"
            "MyEdTSnYSnC7NIHFfvJFBBYi9kcAVBwkYyALQDv6iTGMSI11/ncwdTz4/GGPodaU"
            "PFxf/WVHUz6rBAtTKvn8Kg61F6cVhcFSCjiw2bWGpeWcWTL+CGbfYCvZNiAVyO7Q"
            "dmfj5hoLu7KG+nxBLF8uoUl6t3BdKz9Dqg9Vf+QVtaVj/COD1nUykXXRVxfLo4dN"
            "BS+aVsmOFjppKaEvmeT5SwWOSVrKZwPuTilV9jCehFbFZF6MPyiW5mcp9t4D27hM"
            "oz/SiKjCqdN93YdBO4FBF/cWD5WHmD7KaaJYmnztz3W+xS7b/qk2PcN+qpZEXsfr"
            "Wie4prB1umESavYLC1pLhoEgc0jRUl1b9mHSY7E4puk="
    };


    void
    clean_state_folder()
    {
        try
        {
            if (boost::filesystem::exists(TEST_STATE_DIR))
            {
                boost::filesystem::remove_all(TEST_STATE_DIR);
            }
            boost::filesystem::create_directory(TEST_STATE_DIR);

            if (boost::filesystem::exists("./.state"))
            {
                boost::filesystem::remove_all("./.state");
            }
            boost::filesystem::create_directory("./.state");
        }
        catch(boost::filesystem::filesystem_error const& e)
        {
            LOG(error) << "Error while attempting to clean the state folder:" << e.what();
        }
    }


    bzn::json_message
    peers_list_to_JSON_array(const bzn::peers_list_t& list_of_peers)
    {
        bzn::json_message root;
        for (const auto& address : list_of_peers)
        {
            bzn::json_message peer;
            peer["port"] = address.port;
            peer["http_port"] = address.http_port;
            peer["host"] = address.host;
            peer["uuid"] = address.uuid;
            peer["name"] = address.name;
            root.append(peer);
        }
        return root;
    }


    bzn::json_message
    get_peer_response_from_leader(const bzn::peers_list_t& list_of_peers)
    {
        bzn::json_message response;
        response["bzn-api"] = "raft";
        response["cmd"] = "get_peers_response";
        response["from"] = LEADER_NODE_UUID;
        response["message"] = peers_list_to_JSON_array(list_of_peers);
        return response;
    }


    bzn::json_message
    get_peer_response_from_follower()
    {
        bzn::json_message response;
        response["bzn-api"] = "raft";
        response["cmd"] = "get_peers_response";
        response["from"] = CANDIDATE_NODE_UUID;
        response["error"] = ERROR_GET_PEERS_MUST_BE_SENT_TO_LEADER;
        response["message"]["leader"]["uuid"] = LEADER_NODE_UUID;
        return response;
    }


    bzn::json_message
    get_peer_response_from_candidate()
    {
        bzn::json_message response;
        response["bzn-api"] = "raft";
        response["cmd"] = "get_peers_response";
        response["from"] = CANDIDATE_NODE_UUID;
        response["error"] = ERROR_GET_PEERS_ELECTION_IN_PROGRESS_TRY_LATER ;
        return response;
    }
}


namespace bzn
{
    class raft_peers_test : public Test
    {
    public:
        raft_peers_test()
        {}


        void SetUp() final
        {
            clean_state_folder();
            this->mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
            this->mock_node = std::make_shared<bzn::Mocknode_base>();
            this->mock_session = std::make_shared<bzn::Mocksession_base>();
        }


        void TearDown() final
        {
            clean_state_folder();
        }

        std::shared_ptr<bzn::Mocknode_base> mock_node;
        std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context;
        std::shared_ptr<bzn::Mocksession_base> mock_session;
    };


    TEST_F(raft_peers_test, test_that_raft_doesn_t_attempt_auto_add_if_already_in_quorum)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(2);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(2);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        // We need to fake the other nodes by mocking thier responses to send messages.
        // There are three options:
        //  1) I'm the leader here are the peers
        //  2) I'm the follower, here's the leader
        //  3) I'm a candidate, there might be an election happening, try later
        // We will handle the first option:
        EXPECT_CALL(*this->mock_node, send_message(_, _)).WillRepeatedly( Invoke(
                [&](const boost::asio::ip::tcp::endpoint& /*ep*/, std::shared_ptr<bzn::json_message> msg)
                {
                    const auto request = *msg;
                    if (request["cmd"].asString() == "get_peers")
                    {
                        LOG(debug) << "Mock Leader Raft recieved get_peers, sending peers";
                        mh(get_peer_response_from_leader(FULL_TEST_PEER_LIST), this->mock_session);
                    }
                }));

        ///////////////////////////////////////////////////////////////////////
        // create raft...
        auto raft = std::make_shared<bzn::raft>(
                this->mock_io_context,
                mock_node,
                PARTIAL_TEST_PEER_LIST,
                TEST_NODE_UUID, TEST_STATE_DIR);

        // and away we go...
        raft->start();
        EXPECT_FALSE(raft->in_a_swarm);

        // Get the ball rolling buy expiring the election timer...
        wh(boost::system::error_code());

        EXPECT_TRUE(raft->in_a_swarm);
    }


    TEST_F(raft_peers_test, test_that_raft_calls_a_follower_who_knows_the_leader_raft_is_already_in_swarm)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(3);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(3);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).Times(3).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        // We need to fake the other nodes by mocking their responses to send messages.
        // There are three options:
        //  1) I'm the leader here are the peers
        //  2) I'm the follower, here's the leader
        //  3) I'm a candidate, there might be an election happening, try later
        // Handling option 2, ask follower, follower returns Leader, ask leader
        bzn::json_message response;
        EXPECT_CALL(*this->mock_node, send_message(_, _)).WillRepeatedly( Invoke(
                [&](const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::json_message> msg)
                {
                    const auto request = *msg;
                    LOG(debug) << "MOCK RAFT - received cmd:[" << request["cmd"] << "] from " << ep.address().to_string() << ":" << ep.port();
                    if (request["cmd"].asString() == "get_peers")
                    {
                        static size_t count;
                        // Let's handle option 2
                        switch(count)
                        {
                            case 0: // at first raft chose the wrong node, a follower
                                LOG(debug) << "MOCK RAFT - follower raft responding with leader info";
                                EXPECT_EQ( request["cmd"].asString(), "get_peers");
                                response = get_peer_response_from_follower();
                                break;
                            case 1: //the follower told raft who the leader was
                            case 2:
                                LOG(debug) << "MOCK RAFT - leader raft responding with peers list";
                                EXPECT_EQ(request["cmd"].asString(), "get_peers");
                                EXPECT_EQ(ep.port(), 8081); // raft sent msg to leader
                                response = get_peer_response_from_leader(FULL_TEST_PEER_LIST);
                                break;
                            default:
                                EXPECT_TRUE(false); // we should not get here.
                                break;
                        }
                        count++;
                    }
                }));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, mock_node, PARTIAL_TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        // and away we go...
        raft->start();
        EXPECT_FALSE(raft->in_a_swarm);

        // Get the ball rolling buy expiring the election timer...
        wh(boost::system::error_code());

        // Then raft, asked the leader, so we fake the leader sending back a
        // peer list that contains raft.
        mh(response, this->mock_session);

        EXPECT_FALSE(raft->in_a_swarm);

        wh(boost::system::error_code());

        // The leader responded with the list, so
        mh(response, this->mock_session);

        EXPECT_TRUE(raft->in_a_swarm);
    }


    TEST_F(raft_peers_test, test_that_raft_that_is_not_in_a_swarm_will_add_itself)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(2);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(2);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).Times(2).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));


        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));


        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));


        bzn::json_message response;
        // We need to fake the other nodes by mocking their responses to send messages.
        // There are three options:
        //  1) I'm the leader here are the peers
        //  2) I'm the follower, here's the leader
        //  3) I'm a candidate, there might be an election happening, try later
        EXPECT_CALL(*this->mock_node, send_message(_, _)).WillRepeatedly( Invoke(
                [&](const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::json_message> msg)
                {
                    static size_t count;
                    const auto request = *msg;

                    switch(count)
                    {
                        case 0: // at first raft chose the wrong node, a follower
                        LOG(debug) << "MOCK RAFT - follower received get_peers from RAFT";
                        EXPECT_EQ( request["cmd"].asString(), "get_peers");
                        LOG(debug) << "MOCK RAFT - follower telling RAFT who the leader is via get_peers_response";
                        response = get_peer_response_from_follower();
                        break;

                        case 1:
                        case 2:
                            LOG(debug) << "MOCK RAFT - received RequestVote from RAFT - ignoring";
                            break;

                        case 3: //the follower told raft who the leader was
                            LOG(debug) << "MOCK RAFT - leader recieved get_peers from RAFT";
                            EXPECT_EQ(request["cmd"].asString(), "get_peers");
                            EXPECT_EQ(ep.port(), 8081); // raft sent msg to leader
                            LOG(debug) << "MOCK RAFT - leader sending RAFT the peers list, but RAFT is not in it";
                            response = get_peer_response_from_leader(PARTIAL_TEST_PEER_LIST);
                            break;

                        case 4:
                            LOG(debug) << "MOCK RAFT - leader recieved [" << request["cmd"].asString() << "] from RAFT";
                            EXPECT_EQ(request["cmd"].asString(), "add_peer");
                            response["bzn-api"] = "raft";
                            response["cmd"] = "add_peer";
                            response["response"] = bzn::json_message();
                            response["response"]["from"] = LEADER_NODE_UUID;
                            response["response"]["msg"] = SUCCESS_PEER_ADDED_TO_SWARM;
                            break;

                        default:
                            EXPECT_TRUE(false); // we should not get here.
                            break;
                    }
                    count++;
                }));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, mock_node, FULL_TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR, bzn::DEFAULT_MAX_STORAGE_SIZE, true, signature);

        // and away we go...
        raft->start();
        EXPECT_FALSE(raft->in_a_swarm);

        // Get the ball rolling buy expiring the election timer...
        // this will also trigger raft to ask for the list of peers
        wh(boost::system::error_code());

        // Raft asked a follower who told raft whoe hte leader is.
        mh(response, this->mock_session);
        EXPECT_EQ(raft->get_leader().uuid,  LEADER_NODE_UUID);

        // The leader responded with the list, but we should not be in it
        mh(response, this->mock_session);
        EXPECT_FALSE(raft->in_a_swarm);

        // RAFT asked for peers from leader
        mh(response, this->mock_session);
        EXPECT_TRUE(raft->in_a_swarm);
    }


    TEST_F(raft_peers_test, test_that_raft_tries_again_when_encountering_a_candidate)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(3);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(3);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).Times(3).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        bzn::json_message response;
        // We need to fake the other nodes by mocking their responses to send messages.
        // There are three options:
        //  1) I'm the leader here are the peers
        //  2) I'm the follower, here's the leader
        //  3) I'm a candidate, there might be an election happening, try later
        EXPECT_CALL(*this->mock_node, send_message(_, _)).WillRepeatedly( Invoke(
                [&](const boost::asio::ip::tcp::endpoint& /*ep*/, std::shared_ptr<bzn::json_message> msg)
                {
                    static size_t count;
                    const auto request = *msg;

                    switch(count)
                    {
                        case 0: // get_peers
                            LOG (debug) << "MOCK RAFT - received get_peers request, sending candidate error message";
                            EXPECT_EQ( request["cmd"].asString(), "get_peers");
                            response = get_peer_response_from_candidate();
                            break;

                        case 1: // RequestVote - ignore
                        case 2:
                            break;

                        case 3:// get_peers
                            LOG (debug) << "MOCK RAFT - received get_peers request, sending candidate error message";
                            EXPECT_EQ( request["cmd"].asString(), "get_peers");
                            response = get_peer_response_from_candidate();
                            break;

                        case 4:
                        case 5:
                            break;

                        default:
                            EXPECT_TRUE(false); // we should not get here.
                            break;
                    }
                    count++;
                }));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, mock_node, PARTIAL_TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        // and away we go...
        raft->start();
        EXPECT_FALSE(raft->in_a_swarm);

        // Get the ball rolling buy expiring the election timer...
        wh(boost::system::error_code());
        EXPECT_FALSE(raft->in_a_swarm);

        // Then raft, asked the leader, so we fake the leader sending back a
        // peer list that contains raft.
        mh(response, this->mock_session);
        EXPECT_FALSE(raft->in_a_swarm);

        // give it another shot, it should fail
        wh(boost::system::error_code());
        EXPECT_FALSE(raft->in_a_swarm);
    }
}
