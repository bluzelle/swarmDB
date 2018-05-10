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

#include <raft/raft.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_node_base.hpp>
#include <mocks/mock_session_base.hpp>

using namespace ::testing;

namespace
{
    const bzn::uuid_t TEST_NODE_UUID{"f0645cc2-476b-485d-b589-217be3ca87d5"};

    const bzn::peers_list_t TEST_PEER_LIST{{"127.0.0.1", 8081, "name1", "uuid1"},
                                           {"127.0.0.1", 8082, "name2", "uuid2"},
                                           {"127.0.0.1", 8084, "name3", TEST_NODE_UUID}};

    const std::string RAFT_TIMEOUT_SCALE = "RAFT_TIMEOUT_SCALE";
}

namespace bzn
{

    TEST(raft, test_that_default_raft_state_is_follower)
    {
        EXPECT_EQ(bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
            nullptr, TEST_PEER_LIST, TEST_NODE_UUID).get_state(), bzn::raft_state::follower);
    }


    TEST(raft, test_that_a_raft_constructor_throws_when_given_empty_peers_list)
    {
        EXPECT_THROW(bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
            nullptr, {}, TEST_NODE_UUID), std::runtime_error);

        EXPECT_NO_THROW(bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
            nullptr, TEST_PEER_LIST, TEST_NODE_UUID));
    }


    TEST(raft, test_that_start_randomly_schedules_callback_for_starting_an_election_and_wins)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_node = std::make_shared<bzn::Mocknode_base>();
        auto mock_session = std::make_shared<bzn::Mocksession_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(3);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(3);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).Times(3).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        EXPECT_CALL(*mock_node, register_for_message("raft", _));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(mock_io_context, mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

        // and away we go...
        raft->start();

        // we should see requests for votes... and then the Append Requests
        std::vector<bzn::message_handler> mh;
        EXPECT_CALL(*mock_node, send_message(_, _, _)).Times((TEST_PEER_LIST.size() - 1) * 2).WillRepeatedly(Invoke(
            [&](const auto&, const auto&, auto handler)
            { mh.emplace_back(handler); }));

        // expire timer...
        wh(boost::system::error_code());

        // now send in each vote...
        mh[0](bzn::create_request_vote_response("uuid1", 1, true), mock_session);
        mh[1](bzn::create_request_vote_response("uuid2", 1, true), mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);
    }


    TEST(raft, test_that_request_for_vote_response_while_candidate_is_no)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_node = std::make_shared<bzn::Mocknode_base>();
        auto mock_session = std::make_shared<bzn::Mocksession_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(2);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(2);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).Times(2).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(mock_io_context, mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

        // intercept the node raft registration handler...
        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
            [&](const auto&, auto handler)
            {
                mh = handler;
                return true;
            }));

        // and away we go...
        raft->start();

        // don't care about the handler...
        EXPECT_CALL(*mock_node, send_message(_, _, _)).Times(TEST_PEER_LIST.size() - 1);

        // expire timer...
        wh(boost::system::error_code());

        EXPECT_EQ(raft->get_state(), bzn::raft_state::candidate);

        bzn::message resp;
        EXPECT_CALL(*mock_session, send_message(_, _)).WillOnce(Invoke(
            [&](const auto& msg, auto)
            { resp = *msg; }));

        // send a message through the registered "node message" callback...
        mh(bzn::create_request_vote_request("uuid1", 2, 0, 0), mock_session);

        // we expect a "no" response in this state...
        EXPECT_EQ(resp["cmd"].asString(), "responseVote");
        EXPECT_EQ(resp["data"]["granted"].asBool(), false);
    }


    TEST(raft, test_that_in_a_leader_state_will_send_a_heartbeat_to_its_peers)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_node = std::make_shared<bzn::Mocknode_base>();
        auto mock_session = std::make_shared<bzn::Mocksession_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(4);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(4);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).Times(4).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        EXPECT_CALL(*mock_node, register_for_message("raft", _));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(mock_io_context, mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

        // and away we go...
        raft->start();

        // we should see requests for votes...
        std::vector<bzn::message_handler> mh_req;
        EXPECT_CALL(*mock_node, send_message(_, _, _)).Times(TEST_PEER_LIST.size() - 1).WillRepeatedly(Invoke(
            [&](const auto&, const auto& msg, auto handler)
            {
                EXPECT_EQ((*msg)["cmd"].asString(), "RequestVote");
                mh_req.emplace_back(handler);
            }));

        // expire election timer...
        wh(boost::system::error_code());

        // heartbeat timer expired and we should be sending requests...
        std::vector<bzn::message_handler> mh_resp;
        EXPECT_CALL(*mock_node, send_message(_, _, _)).Times((TEST_PEER_LIST.size() - 1) * 2).WillRepeatedly(Invoke(
            [&](const auto&, const auto& msg, auto handler)
            {
                EXPECT_EQ((*msg)["cmd"].asString(), "AppendEntries");
                mh_resp.emplace_back(handler);
            }));

        // now send in each vote...
        mh_req[0](bzn::create_request_vote_response("uuid1", 1, true), mock_session);
        mh_req[1](bzn::create_request_vote_response("uuid2", 1, true), mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        // expire heartbeat timer...
        wh(boost::system::error_code());
    }


    TEST(raft, test_that_as_follower_append_entries_is_answered)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_node = std::make_shared<bzn::Mocknode_base>();
        auto mock_session = std::make_shared<bzn::Mocksession_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(2);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(2);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).Times(2).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
            [&](const auto&, auto handler)
            {
                mh = handler;
                return true;
            }));

        auto raft = std::make_shared<bzn::raft>(mock_io_context, mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

        raft->start();

        bzn::message resp;
        EXPECT_CALL(*mock_session, send_message(_, _)).Times(1).WillRepeatedly(Invoke(
            [&](const auto& msg, auto)
            { resp = *msg; }));

        // send a message through the registered "node message" callback...
        bzn::message msg;
        mh(bzn::create_append_entries_request("uuid", 1, 0, 0, 0, 0, msg), mock_session);

        // we expect a "no" response in this state...
        EXPECT_EQ(resp["cmd"].asString(), "AppendEntriesReply");
        EXPECT_EQ(resp["data"]["success"].asBool(), true);
    }


    TEST(raft, test_that_leader_sends_entries_and_commits_when_enough_peers_have_saved_them)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_node = std::make_shared<NiceMock<bzn::Mocknode_base>>();
        auto mock_session = std::make_shared<bzn::Mocksession_base>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        EXPECT_CALL(*mock_node, register_for_message("raft", _));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(mock_io_context, mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

        // and away we go...
        raft->start();

        EXPECT_EQ(raft->get_leader().uuid, "");

        // try to append a log. It will fail since we are not the leader...
        ASSERT_FALSE(raft->append_log(bzn::message()));

        EXPECT_EQ(raft->get_state(), bzn::raft_state::follower);

        // we should see requests for votes...
        std::vector<bzn::message_handler> send_messages;
        EXPECT_CALL(*mock_node, send_message(_, _, _)).WillRepeatedly(Invoke(
            [&](const auto&, const auto& /*msg*/, auto handler)
            {
                LOG(info) << send_messages.size();// << ":\n" << msg.toStyledString();
                send_messages.emplace_back(handler);
            }));

        // expire election timer...
        wh(boost::system::error_code());

        EXPECT_EQ(raft->get_state(), bzn::raft_state::candidate);

        // now send in each vote...
        send_messages[0](bzn::create_request_vote_response("uuid1", 1, true), mock_session);
        send_messages[1](bzn::create_request_vote_response("uuid2", 1, true), mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        EXPECT_EQ(raft->get_leader().uuid, TEST_NODE_UUID);

        ///////////////////////////////////////////////////////////////////////////

        bool commit_handler_called = false;
        int commit_handler_times_called = 0;
        raft->register_commit_handler(
            [&](const bzn::message& msg)
            {
                LOG(info) << "commit:\n" << msg.toStyledString();

                commit_handler_called = true;
                ++commit_handler_times_called;
                return true;
            });

        // create a log entry now that we are the leader...
        bzn::message msg;
        msg["bzn-api"] = "crud";
        msg["data"] = "utests_1";
        ASSERT_TRUE(raft->append_log(msg));
        msg["data"] = "utests_2";
        ASSERT_TRUE(raft->append_log(msg));

        // clear stored callbacks...
        send_messages.clear();

        // next heartbeat...
        wh(boost::system::error_code());

        EXPECT_EQ(send_messages.size(), size_t(2));

        // send false so second peer will achieve consensus and leader will commit the entries..
        send_messages[0](bzn::create_append_entries_response("uuid1", 2, false, 0), mock_session);

        EXPECT_EQ(commit_handler_times_called, 0);
        ASSERT_FALSE(commit_handler_called);

        // enough peers have stored the first entry
        send_messages[1](bzn::create_append_entries_response("uuid2", 2, true, 1), mock_session);

        EXPECT_EQ(commit_handler_times_called, 1);

        // clear stored callbacks...
        send_messages.clear();

        // expire heart beat
        wh(boost::system::error_code());

        EXPECT_EQ(send_messages.size(), size_t(2));

        // enough peers have stored the first entry
        commit_handler_times_called = 0;
        commit_handler_called = false;
        send_messages[1](bzn::create_append_entries_response("uuid2", 2, true, 2), mock_session);

        EXPECT_EQ(commit_handler_times_called, 1);
        ASSERT_TRUE(commit_handler_called);

        // expire heart beat
        wh(boost::system::error_code());

        // todo: verify append entry contents
    }


    TEST(raft, test_that_follower_stores_append_entries_and_responds)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_node = std::make_shared<NiceMock<bzn::Mocknode_base>>();
        auto mock_session = std::make_shared<bzn::Mocksession_base>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(mock_io_context, mock_node, TEST_PEER_LIST, "uuid1");

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
            [&](const auto&, auto handler)
            {
                mh = handler;
                return true;
            }));

        // and away we go...
        raft->start();

        EXPECT_EQ(raft->get_leader().uuid, "");

        // try to append a log. It will fail since we are not the leader...
        ASSERT_FALSE(raft->append_log(bzn::message()));

        EXPECT_EQ(raft->get_state(), bzn::raft_state::follower);

        ///////////////////////////////////////////////////////////////////////////

        bzn::message resp;
        EXPECT_CALL(*mock_session, send_message(_, _)).WillRepeatedly(Invoke(
            [&](const auto& msg, auto /*handler*/)
            {
                resp = *msg;
            }));

        int commit_handler_times_called = 0;
        raft->register_commit_handler(
            [&](const bzn::message& msg)
            {
                LOG(info) << "commit:\n" << msg.toStyledString();
                ++commit_handler_times_called;
                return true;
            });

        ///////////////////////////////////////////////////////////////////////////
        bzn::message entry;
        entry["bzn-api"] = "utest";

        auto msg = bzn::create_append_entries_request(TEST_NODE_UUID, 2, 0, 0, 0, 0, bzn::message());
        mh(msg, mock_session);
        ASSERT_TRUE(resp["data"]["success"].asBool());

        resp.clear();

        // send append entry with commit index of 1... and follower commits and updates its match index
        msg = bzn::create_append_entries_request(TEST_NODE_UUID, 2, 1, 0, 0, 2, entry);
        mh(msg, mock_session);
        EXPECT_EQ(resp["data"]["matchIndex"].asUInt(), Json::UInt(1));
        ASSERT_TRUE(resp["data"]["success"].asBool());
        EXPECT_EQ(commit_handler_times_called, 1);

        resp.clear();

        // send invalid entry. peer should return false and not move back past commit index
        msg = bzn::create_append_entries_request(TEST_NODE_UUID, 2, 0, 0, 0, 0, entry);
        mh(msg, mock_session);
        ASSERT_FALSE(resp["data"]["success"].asBool());
        EXPECT_EQ(resp["data"]["matchIndex"].asUInt(), Json::UInt(1));

        resp.clear();

        // put back append entries - bad append entry
        msg = bzn::create_append_entries_request(TEST_NODE_UUID, 2, 1, 0, 0, 2, entry);
        mh(msg, mock_session);
        ASSERT_FALSE(resp["data"]["success"].asBool());
        EXPECT_EQ(resp["data"]["matchIndex"].asUInt(), Json::UInt(1));
    }


    TEST(raft, test_raft_timeout_scale_can_get_set)
    {
        // none set
        {
            unsetenv(RAFT_TIMEOUT_SCALE.c_str());
            bzn::raft r(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID);
            EXPECT_EQ(r.timeout_scale, 1ul);
        }

        // valid
        {
            setenv(RAFT_TIMEOUT_SCALE.c_str(), "2", 1);
            bzn::raft r(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID);
            EXPECT_EQ(r.timeout_scale, 2ul);
        }

        // invalid
        {
            setenv(RAFT_TIMEOUT_SCALE.c_str(), "asdf", 1);
            bzn::raft r(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID);
            EXPECT_EQ(r.timeout_scale, 1ul);
        }
    }

} // bzn