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
#include <storage/storage.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <random>
#include <stdlib.h>

using namespace ::testing;

namespace
{
    const bzn::uuid_t TEST_NODE_UUID{"f0645cc2-476b-485d-b589-217be3ca87d5"};

    const bzn::peer_address_t new_peer{"127.0.0.1", 8090, 83, "name_new", "uuid_new"};

    const bzn::peers_list_t TEST_PEER_LIST{{"127.0.0.1", 8081, 80, "name1", "uuid1"},
                                           {"127.0.0.1", 8082, 81, "name2", "uuid2"},
                                           {"127.0.0.1", 8084, 82, "name3", TEST_NODE_UUID}};

    const std::string RAFT_TIMEOUT_SCALE = "RAFT_TIMEOUT_SCALE";

    const std::string TEST_STATE_DIR = "./.raft_test_state/";

    void
    fill_entries_with_test_data(const size_t sz, std::vector<bzn::log_entry>& entries)
    {
        if(entries.size() != sz)
        {
            entries.resize(entries.size() + sz);
        }

        uint16_t index = 1;

        std::vector<bzn::log_entry>::iterator start = entries.begin() + (entries.front().entry_type == bzn::log_entry_type::single_quorum ? 1 : 0);

        std::for_each(start,
                      entries.end(),
                      [&](auto &entry)
                      {
                          entry.log_index = index;
                          entry.term = std::div(index, 5).quot;
                          entry.entry_type = bzn::log_entry_type::database;
                          std::string data(200 * (index % 3 + 1), 's');
                          entry.msg["data"]["value"] = data;
                          index++;
                      });
    }


    void
    save_entries_to_path(const std::string& path, const std::vector<bzn::log_entry>& entries)
    {
        std::ofstream ofs(path, std::ios::out |  std::ios::binary | std::ios::app);
        for(auto& entry : entries)
        {
            ofs << entry;
        }
        ofs.close();
    }


    void
    clean_state_folder()
    {
        try
        {
            if(boost::filesystem::exists(TEST_STATE_DIR))
            {
                boost::filesystem::remove_all(TEST_STATE_DIR);
            }
            boost::filesystem::create_directory(TEST_STATE_DIR);

            if(boost::filesystem::exists("./.state"))
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


    auto equality_test = [](const bzn::log_entry &rhs, const bzn::log_entry &lhs) -> bool
    {
        return rhs.log_index == lhs.log_index
               && rhs.term == lhs.term
               && rhs.msg.toStyledString() == lhs.msg.toStyledString();
    };


    bzn::message
    make_add_peer_request()
    {
        bzn::message  msg;
        msg["bzn-api"] = "raft";
        msg["cmd"] = "add_peer";
        msg["data"]["peer"]["name"] = new_peer.name;
        msg["data"]["peer"]["host"] = new_peer.host;
        msg["data"]["peer"]["port"] = new_peer.port;
        msg["data"]["peer"]["http_port"] = new_peer.http_port;
        msg["data"]["peer"]["uuid"] = new_peer.uuid;
        return msg;
    }


    bzn::message
    make_duplicate_add_peer_request()
    {
        bzn::message  msg{make_add_peer_request()};
        msg["data"]["peer"]["uuid"] = TEST_PEER_LIST.begin()->uuid;
        return msg;
    }


    bzn::message
    make_remove_peer_request()
    {
        bzn::message  msg;
        msg["bzn-api"] = "raft";
        msg["cmd"] = "remove_peer";
        msg["data"]["uuid"] = TEST_NODE_UUID;
        return msg;
   }


    bzn::message
    make_remove_nonexisting_peer_request()
    {
        bzn::message  msg;
        msg["bzn-api"] = "raft";
        msg["cmd"] = "remove_peer";
        msg["data"]["uuid"] = "uuid_does_not_exist";
        return msg;

    }


    void
    json_to_peers(const bzn::message &peers, bzn::peers_list_t &peers_list)
    {
        std::for_each(peers.begin(), peers.end(),
                      [&](const auto &p)
                      {
                          peers_list.emplace(
                                  p["host"].asString(),
                                  uint16_t(p["port"].asUInt()),
                                  uint16_t(p["http_port"].asUInt()),
                                  p["name"].asString(),
                                  p["uuid"].asString());
                      });
    }


    bzn::message
    make_dummy_peer()
    {
        bzn::message peer;
        peer["host"] = "127.0.0.1";
        peer["port"] = 8081;
        peer["http_port"] = 81;
        peer["name"] = "tux";
        peer["uuid"] = TEST_NODE_UUID;
        return peer;
    }
}


class MSG_ERROR_ENCOUNTERED_INVALID_ENTRY_IN_LOG;
namespace bzn
{
    class raft_test : public Test
    {
    public:
        raft_test()
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

    TEST(raft, test_that_default_raft_state_is_follower)
    {
        clean_state_folder();
        auto sut = bzn::raft(
                std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>()
                ,nullptr
                , TEST_PEER_LIST
                , TEST_NODE_UUID
                , TEST_STATE_DIR
        );
        EXPECT_EQ(sut.get_state(), bzn::raft_state::follower);
        clean_state_folder();
    }


    TEST(raft, test_that_a_raft_constructor_throws_when_given_empty_peers_list)
    {
        EXPECT_THROW(bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
                               nullptr, {}, TEST_NODE_UUID, TEST_STATE_DIR), std::runtime_error);

        clean_state_folder();
        EXPECT_NO_THROW(bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
                                  nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR));
    }


    TEST_F(raft_test, test_that_start_randomly_schedules_callback_for_starting_an_election_and_wins)
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

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        bzn::message_handler mh;
        EXPECT_CALL(*this->mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));;

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        // and away we go...
        raft->start();

        // we should see requests for votes... and then the Append Requests
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times((TEST_PEER_LIST.size() - 1) * 2);

        // expire timer...
        wh(boost::system::error_code());

        // now send in each vote...
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), this->mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), this->mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);
    }


    TEST_F(raft_test, test_that_request_for_vote_response_while_candidate_is_no)
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

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        // intercept the node raft registration handler...
        bzn::message_handler mh;
        EXPECT_CALL(*this->mock_node, register_for_message("raft", _)).WillOnce(Invoke(
            [&](const auto&, auto handler)
            {
                mh = handler;
                return true;
            }));

        // and away we go...
        raft->start();

        // don't care about the handler...
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times(TEST_PEER_LIST.size() - 1);

        // expire timer...
        wh(boost::system::error_code());

        EXPECT_EQ(raft->get_state(), bzn::raft_state::candidate);

        bzn::message resp;
        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(), _)).WillOnce(Invoke(
            [&](const auto& msg, auto)
            { resp = *msg; }));

        // send a message through the registered "node message" callback...
        mh(bzn::create_request_vote_request("uuid1", 1, 0, 0), mock_session);

        // we expect a "no" response in this state...
        EXPECT_EQ(resp["cmd"].asString(), "ResponseVote");
        EXPECT_EQ(resp["data"]["granted"].asBool(), false);
    }


    TEST_F(raft_test, test_that_in_a_leader_state_will_send_a_heartbeat_to_its_peers)
    {
        auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();

        // timer expectations...
        EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(4);
        EXPECT_CALL(*mock_steady_timer, cancel()).Times(4);

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).Times(4).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        EXPECT_CALL(*this->mock_node, register_for_message("raft", _));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
        raft->enable_audit = false;

        // and away we go...
        raft->start();

        // we should see requests for votes...
        std::vector<bzn::message_handler> mh_req;
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times(TEST_PEER_LIST.size() - 1).WillRepeatedly(Invoke(
            [&](const auto&, const auto& msg)
            {
                EXPECT_EQ((*msg)["cmd"].asString(), "RequestVote");
            }));

        // expire election timer...
        wh(boost::system::error_code());

        // heartbeat timer expired and we should be sending requests...
        std::vector<bzn::message_handler> mh_resp;
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times((TEST_PEER_LIST.size() - 1) * 2).WillRepeatedly(Invoke(
            [&](const auto&, const auto& msg)
            {
                EXPECT_EQ((*msg)["cmd"].asString(), "AppendEntries");
            }));

        // now send in each vote...
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        // expire heartbeat timer...
        wh(boost::system::error_code());
    }


    TEST_F(raft_test, test_that_as_follower_append_entries_is_answered)
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

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        bzn::message_handler mh;
        EXPECT_CALL(*this->mock_node, register_for_message("raft", _)).WillOnce(Invoke(
            [&](const auto&, auto handler)
            {
                mh = handler;
                return true;
            }));

        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        raft->start();

        bzn::message resp;
        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(), _)).Times(1).WillRepeatedly(Invoke(
            [&](const auto& msg, auto)
            { resp = *msg; }));

        // send a message through the registered "node message" callback...
        bzn::message msg;
        mh(bzn::create_append_entries_request("uuid2"
                                              , 0 // current term ok
                                              , 0 // commit index doesn't matter
                                              , 0 // previous index ok
                                              , 0 // previous term ok
                                              , 0 // entry term doesn't matter
                                              , msg), mock_session);

        EXPECT_EQ(resp["cmd"].asString(), "AppendEntriesReply");
        EXPECT_EQ(resp["data"]["success"].asBool(), true);
    }


    TEST_F(raft_test, test_that_leader_sends_entries_and_commits_when_enough_peers_have_saved_them)
    {
        boost::filesystem::remove(TEST_STATE_DIR + TEST_NODE_UUID + ".dat");
        boost::filesystem::remove(TEST_STATE_DIR + TEST_NODE_UUID + ".state");
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        EXPECT_CALL(*this->mock_node, register_for_message("raft", _));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
        raft->enable_audit = false;

        // and away we go...
        raft->start();

        EXPECT_EQ(raft->get_leader().uuid, "");

        // try to append a log. It will fail since we are not the leader...
        ASSERT_FALSE(raft->append_log(bzn::message(), bzn::log_entry_type::database));

        EXPECT_EQ(raft->get_state(), bzn::raft_state::follower);

        // we should see requests...
        EXPECT_CALL(*mock_node, send_message(_, _)).Times(10);

        // expire election timer...
        wh(boost::system::error_code());

        EXPECT_EQ(raft->get_state(), bzn::raft_state::candidate);

        // now send in each vote...
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        EXPECT_EQ(raft->get_leader().uuid, TEST_NODE_UUID);

        bool commit_handler_called = false;
        int commit_handler_times_called = 0;
        raft->register_commit_handler(
            [&](const bzn::message& msg)
            {
                LOG(info) << "commit:\n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

                commit_handler_called = true;
                ++commit_handler_times_called;
                return true;
            });

        // create a log entry now that we are the leader...
        bzn::message msg;
        msg["bzn-api"] = "crud";
        msg["data"] = "utests_1";
        ASSERT_TRUE(raft->append_log(msg, bzn::log_entry_type::database));
        msg["data"] = "utests_2";
        ASSERT_TRUE(raft->append_log(msg, bzn::log_entry_type::database));

        // next heartbeat...
        wh(boost::system::error_code());

        // send false so second peer will achieve consensus and leader will commit the entries..
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid1", 1, false, 1), mock_session);

        EXPECT_EQ(commit_handler_times_called, 0);
        ASSERT_FALSE(commit_handler_called);

        // enough peers have stored the first entry
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid2", 1, true, 2), this->mock_session);

        EXPECT_EQ(commit_handler_times_called, 1);

        // expire heart beat
        wh(boost::system::error_code());

        // enough peers have stored the first entry
        commit_handler_times_called = 0;
        commit_handler_called = false;
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid2", 1, true, 3), this->mock_session);

        EXPECT_EQ(commit_handler_times_called, 1);
        ASSERT_TRUE(commit_handler_called);

        // expire heart beat
        wh(boost::system::error_code());

        // todo: verify append entry contents
    }


    TEST_F(raft_test, test_that_follower_stores_append_entries_and_responds)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
            [&](auto handler)
            { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            { return std::move(mock_steady_timer); }));

        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, "uuid1", TEST_STATE_DIR);

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
        ASSERT_FALSE(raft->append_log(bzn::message(), bzn::log_entry_type::database));

        EXPECT_EQ(raft->get_state(), bzn::raft_state::follower);

        ///////////////////////////////////////////////////////////////////////////

        bzn::message resp;
        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(), _)).WillRepeatedly(Invoke(
            [&](const auto& msg, auto /*handler*/)
            {
                resp = *msg;
            }));

        int commit_handler_times_called = 0;
        raft->register_commit_handler(
            [&](const bzn::message& msg)
            {
                LOG(info) << "commit:\n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";
                ++commit_handler_times_called;
                return true;
            });

        ///////////////////////////////////////////////////////////////////////////
        bzn::message entry;
        entry["bzn-api"] = "utest";

        auto msg = bzn::create_append_entries_request(TEST_NODE_UUID
                , 2 // current term
                , 1 // commit index
                , 0 // previous index
                , 0 // previous term
                , 0 // entry term
                , bzn::message());
        mh(msg, this->mock_session);

        resp.clear();

        // send append entry with commit index of 3... and follower commits and updates its match index
        msg = bzn::create_append_entries_request(TEST_NODE_UUID, 2, 2, 0, 0, 2, entry);
        mh(msg, this->mock_session);
        EXPECT_EQ(resp["data"]["matchIndex"].asUInt(), Json::UInt(2));
        ASSERT_TRUE(resp["data"]["success"].asBool());
        EXPECT_EQ(commit_handler_times_called, 1);

        resp.clear();
        // send invalid entry. peer should return false and not move back past commit index
        msg = bzn::create_append_entries_request(TEST_NODE_UUID
                , 2 // current term is ok
                , 3 // commit index is ok
                , 1 // previous index is ok
                , 0 // previous term is wrong
                , 2 // entry term is ok
                , entry);
        mh(msg, mock_session);
        ASSERT_FALSE(resp["data"]["success"].asBool());
        EXPECT_EQ(resp["data"]["matchIndex"].asUInt(), Json::UInt(2));
        EXPECT_EQ(commit_handler_times_called, 1);

        resp.clear();

        // put back append entries - bad append entry
        msg = bzn::create_append_entries_request(TEST_NODE_UUID
                , 0 // current term is wrong
                , 3 // commit index is OK
                , 1 // previous index is OK
                , 2 // previous term is OK
                , 2 // entry term is OK
                , entry);
        mh(msg, this->mock_session);

        ASSERT_FALSE( resp["data"].isMember("success") && resp["data"]["success"].asBool());
        EXPECT_EQ(commit_handler_times_called, 1);
    }


    TEST(raft, test_raft_timeout_scale_can_get_set)
    {
        // none set
        {
            clean_state_folder();
            unsetenv(RAFT_TIMEOUT_SCALE.c_str());
            bzn::raft r(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
            EXPECT_EQ(r.timeout_scale, 1ul);
        }

        // valid
        {
            clean_state_folder();
            setenv(RAFT_TIMEOUT_SCALE.c_str(), "2", 1);
            bzn::raft r(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
            EXPECT_EQ(r.timeout_scale, 2ul);
        }

        // invalid
        {
            clean_state_folder();
            setenv(RAFT_TIMEOUT_SCALE.c_str(), "asdf", 1);
            bzn::raft r(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
            EXPECT_EQ(r.timeout_scale, 1ul);
        }
        clean_state_folder();
    }


    TEST(raft, test_log_entry_serialization)
    {
        const size_t number_of_entries = 3;
        const std::string path{TEST_STATE_DIR + "test_data.dat"};
        clean_state_folder();

        std::vector<bzn::log_entry> entries_source(number_of_entries);
        fill_entries_with_test_data(number_of_entries, entries_source);
        save_entries_to_path(path, entries_source);

        std::vector<bzn::log_entry> entries_target;
        {
            bzn::log_entry log_entry;
            std::ifstream is(path, std::ios::binary | std::ios::in);

            while (is >> log_entry)
            {
                if (!log_entry.msg.empty())
                {
                    entries_target.emplace_back(log_entry);
                }
            }
        }

        EXPECT_EQ(number_of_entries, entries_target.size());
        EXPECT_TRUE(std::equal(entries_source.begin(), entries_source.end(), entries_target.begin(), equality_test));

        clean_state_folder();
    }

    // TODO: The following test needs to be rewritten
//    TEST(raft, test_that_raft_can_rehydrate_state_and_log_entries)
//    {
//        clean_state_folder();
//
//        auto mock_session = std::make_shared<bzn::Mocksession_base>();
//        auto raft_source = std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
//
//        const size_t number_of_entries = 300;
//        // fill_entries_with_test_data(number_of_entries, raft_source->raft_log.get_log_entries());
//        initilialize_raft_log(number_of_entries, raft_source);
//        // TODO: this should be done via RAFT, not by cheating. That is, I'd like to simulate the append entry process to ensure that append_entry_to_log gets called
//        std::for_each(raft_source->raft_log->get_log_entries().begin(), raft_source->raft_log->get_log_entries().end(),
//                      [&](const auto &database)
//                      {
//                          if (database.log_index > 0)
//                          {
//                              raft_source->append_log(database.msg, database.entry_type);
//                          }
//                      });
//
//        raft_source->last_log_term = raft_source->raft_log->get_log_entries().back().term;
//        raft_source->commit_index = raft_source->raft_log->get_log_entries().back().log_index;
//        raft_source->current_term = raft_source->last_log_term;
//
//        // TODO: This should be done via RAFT, not by cheating.
//        raft_source->save_state();
//
//        // instantiate a raft with the same uuid
//        auto raft_target = std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
//
//        EXPECT_EQ(raft_target->raft_log->get_log_entries().size(), raft_source->raft_log->get_log_entries().size());
//        EXPECT_EQ(raft_target->last_log_term, raft_source->last_log_term);
//        EXPECT_EQ(raft_target->commit_index, raft_source->commit_index);
//        EXPECT_EQ(raft_target->current_term, raft_source->current_term);
//
//        auto source_entries = raft_source->raft_log->get_log_entries();
//        auto target_entries = raft_target->raft_log->get_log_entries();
//
//        EXPECT_TRUE(target_entries.size()>0);
//        EXPECT_EQ(target_entries.size(),source_entries.size());
//
//        EXPECT_TRUE(std::equal(
//                std::begin(target_entries), std::end(target_entries),
//                std::begin(source_entries),
//                [](const bzn::database &rhs, const bzn::database &lhs) -> bool
//                {
//                    return (rhs.log_index == lhs.log_index
//                            && rhs.term == lhs.term
//                            && rhs.msg.toStyledString() == lhs.msg.toStyledString()
//                    );
//                }
//        ));
//
//        clean_state_folder();
//    }

    
    TEST(raft, test_that_raft_can_rehydrate_storage)
    {
        const size_t number_of_entries = 300;

        clean_state_folder();

        auto mock_session = std::make_shared<bzn::Mocksession_base>();
        auto raft_source = std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
                                                       nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
        auto storage_source = std::make_shared<bzn::storage>();

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, 6);

        bzn::message message;
        Json::Reader reader;
        std::string crud_msg;
        uint16_t index = 0;

        raft_source->current_state = bzn::raft_state::leader;

        while (storage_source->get_keys(TEST_NODE_UUID).size() < number_of_entries)
        {
            uint8_t command = dis(gen);
            if (command <= 3)
            {
                // create
                const std::string key = "key" + std::to_string(index);
                const std::string value = "value_for_" + key + "==";

                crud_msg = R"({"bzn-api":"crud","cmd":"create","data":{"key": ")"
                           + key
                           + R"(","value":")"
                           + value
                           + R"("},"db-uuid":")"
                           + TEST_NODE_UUID
                           + R"(","request-id":"0"}")";

                reader.parse(crud_msg, message);
                raft_source->append_log(message, bzn::log_entry_type::database);
                storage_source->create(TEST_NODE_UUID, key, value);
            }
            else if (command < 5)
            {
                // update an existing record
                const auto keys = storage_source->get_keys(TEST_NODE_UUID);
                if (keys.size() > 10)
                {
                    std::uniform_int_distribution<> dis(0, keys.size() - 1);
                    const size_t i = dis(gen);
                    const auto key = keys[i];
                    const auto value = "updated_value_for_" + key;
                    crud_msg = R"({"bzn-api":"crud","cmd":"update","data":{"key": ")"
                               + key
                               + R"(","value":")"
                               + value
                               + R"("},"db-uuid":")"
                               + TEST_NODE_UUID
                               + R"(","request-id":"0"}")";

                    reader.parse(crud_msg, message);
                    raft_source->append_log(message, bzn::log_entry_type::database);
                    storage_source->update(TEST_NODE_UUID, key, value);
                }
            }
            else
            {
                // delete an existing record
                const auto keys = storage_source->get_keys(TEST_NODE_UUID);
                if (keys.size() > 10)
                {
                    std::uniform_int_distribution<> dis(0, keys.size() - 1);
                    const auto key_index = dis(gen);
                    const auto key = keys[key_index];
                    crud_msg = R"({"bzn-api":"crud","cmd":"delete","data":{"key": ")"
                               + key
                               + R"("},"db-uuid":")"
                               + TEST_NODE_UUID
                               + R"(","request-id":"0"}")";

                    reader.parse(crud_msg, message);
                    raft_source->append_log(message, bzn::log_entry_type::database);
                    storage_source->remove(TEST_NODE_UUID, key);
                }
            }
            index++;
        }

        EXPECT_EQ(storage_source->get_keys(TEST_NODE_UUID).size(), number_of_entries);

        auto storage_target = std::make_shared<bzn::storage>();

        raft_source->initialize_storage_from_log(storage_target);

        EXPECT_TRUE(storage_target->get_keys(TEST_NODE_UUID).size() > 0);
        EXPECT_EQ(storage_source->get_keys(TEST_NODE_UUID).size(), storage_target->get_keys(TEST_NODE_UUID).size());

        for (auto& key : storage_source->get_keys(TEST_NODE_UUID))
        {
            auto rec_1 = storage_source->read(TEST_NODE_UUID, key);
            auto rec_2 = storage_target->read(TEST_NODE_UUID, key);
            EXPECT_EQ(rec_1->value, rec_2->value);
        }

        clean_state_folder();
    }


    TEST(raft, test_that_raft_bails_on_bad_rehydrate)
    {
        const std::string state_path{TEST_STATE_DIR + TEST_NODE_UUID + ".state"};
        const std::string log_path{TEST_STATE_DIR + TEST_NODE_UUID + ".dat"};

        const std::string good_state{"1 0 1 4"};
        const std::string bad_state{"X 0 X X"};

        const std::string bad_entry_00{"1 4 THIS_IS_BADeyJiem4tYXB0K"};

        bzn::message sq_msg;
        sq_msg["msg"]["peers"].append(make_dummy_peer());
        const bzn::log_entry valid_entry{bzn::log_entry_type::single_quorum, 0, 0, sq_msg};

        clean_state_folder();

        // good state/ empty entry
        std::ofstream out(state_path, std::ios::out | std::ios::binary);
        out << good_state;
        out.close();

        out.open(log_path, std::ios::out | std::ios::binary);
        out << "";
        out.close();

        EXPECT_THROW(std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR)
                        , std::runtime_error);

        // good state/bad entry
        out.open(log_path, std::ios::out | std::ios::binary);
        out << bad_entry_00;
        out.close();

        bzn::peers_list_t empty_peers;

        EXPECT_THROW(
                std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, empty_peers, TEST_NODE_UUID, TEST_STATE_DIR)
                        , std::runtime_error);

        // TODO: Revist this test
//        // good state/good entry/mis-matched
//        out.open(log_path, std::ios::out | std::ios::binary);
//        out << valid_entry;
//        out.close();
//
//        EXPECT_THROW(
//                std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR)
//                        , std::runtime_error);

        clean_state_folder();
    }


    TEST(raft, test_raft_can_find_last_quorum_log_entry)
    {
        const std::string log_path = TEST_STATE_DIR + "/" + TEST_NODE_UUID + ".dat";
        bzn::message  msg;
        msg["data"] = "asd;lkfa;lsdfjafs;dlfk";

        clean_state_folder();
        {
            auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

            // there must be exactly one entry, and that entry is a single quorum
            const auto quorum = raft.raft_log->last_quorum_entry();
            EXPECT_EQ(quorum.entry_type, bzn::log_entry_type::single_quorum);
            EXPECT_EQ((uint32_t)0, quorum.log_index);
            EXPECT_EQ((uint32_t)0, quorum.term);
        }
        {
            // append a few entries onto the existing log
            std::ofstream log(log_path, std::ios::out | std::ios::binary | std::ios::app);

            log << bzn::log_entry{bzn::log_entry_type::database, 1, 1, msg};
            log << bzn::log_entry{bzn::log_entry_type::database, 2, 1, msg};
            log << bzn::log_entry{bzn::log_entry_type::database, 3, 1, msg};
            log << bzn::log_entry{bzn::log_entry_type::database, 4, 1, msg};
            log.close();
            auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
            const auto quorum = raft.raft_log->last_quorum_entry();
            EXPECT_EQ(quorum.entry_type, bzn::log_entry_type::single_quorum);
            EXPECT_EQ((uint32_t)0, quorum.log_index);
            EXPECT_EQ((uint32_t)0, quorum.term);
        }
        {
            // add a joint quorum, and then a few more log entries
            std::ofstream log(log_path, std::ios::out | std::ios::binary | std::ios::app);
            bzn::message jq_msg;
            jq_msg["msg"]["peers"]["new"].append(make_dummy_peer());
            jq_msg["msg"]["peers"]["old"].append(make_dummy_peer());
            log << bzn::log_entry{bzn::log_entry_type::joint_quorum, 5, 1, jq_msg};
            log << bzn::log_entry{bzn::log_entry_type::database, 6, 1, msg};
            log << bzn::log_entry{bzn::log_entry_type::database, 7, 1, msg};
            log << bzn::log_entry{bzn::log_entry_type::database, 8, 1, msg};
            log.close();
            auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
            const auto quorum = raft.raft_log->last_quorum_entry();
            EXPECT_EQ(quorum.entry_type, bzn::log_entry_type::joint_quorum);
            EXPECT_EQ((uint32_t) 5, quorum.log_index);
            EXPECT_EQ((uint32_t) 1, quorum.term);
        }
        clean_state_folder();
    }


    TEST_F(raft_test, test_that_raft_first_log_entry_is_the_quorum)
    {
        bzn::message expected;
        auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
        EXPECT_EQ(raft.raft_log->size(), static_cast<size_t>(1));
        EXPECT_EQ(raft.raft_log->get_log_entries().front().entry_type, bzn::log_entry_type::single_quorum);

        bzn::message json_quorum = raft.raft_log->get_log_entries().front().msg["msg"]["peers"];
        EXPECT_EQ(json_quorum.size(), TEST_PEER_LIST.size());

        std::for_each(TEST_PEER_LIST.begin(), TEST_PEER_LIST.end(), [&](const auto& peer)
        {
            const auto& found = std::find_if(
                    json_quorum.begin()
                    , json_quorum.end()
                    , [&](const auto& jp)
                    {
                        return peer.uuid == jp["uuid"].asString();
                    });
            EXPECT_TRUE(found !=  json_quorum.end());
        });
    }


    // Note: There is no need for test_raft_throws_exception_when_no_quorum_can_be_found_in_log as
    // the log is always guaranteed to have the first log entry which will be a quorum based on
    // the initial peer list.


    TEST_F(raft_test, test_that_non_leaders_cannot_add_peers)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        //EXPECT_CALL(*this->mock_node, register_for_message("raft", _));

        // craft raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        // and away we go...
        raft->start();

        // the current state must be follower
        EXPECT_TRUE(raft->get_state() == bzn::raft_state::follower);
        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                .WillOnce(Invoke(
                        [&](const auto& msg, auto)
                        {
                            auto root = *msg.get();
                            EXPECT_EQ(root["error"].asString(), ERROR_ADD_PEER_MUST_BE_SENT_TO_LEADER);
                        }));

        mh(make_add_peer_request(),this->mock_session);

        // let's try a raft in candidate state, expire timer...
        // the current state must be follower
        // don't care about the handler...
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times(TEST_PEER_LIST.size() - 1);
        wh(boost::system::error_code());
        EXPECT_EQ(raft->get_state(), bzn::raft_state::candidate);

        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                .WillOnce(Invoke(
                        [&](const auto& msg, auto)
                        {
                            auto root = *msg.get();
                            EXPECT_EQ(root["error"].asString(), ERROR_ADD_PEER_MUST_BE_SENT_TO_LEADER);
                        }));

        mh(make_add_peer_request(), this->mock_session);
    }


    TEST_F(raft_test, test_that_non_leaders_cannot_remove_peers)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));


        // craft raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        // and away we go...
        raft->start();

        // the current state must be follower
        EXPECT_TRUE(raft->get_state() == bzn::raft_state::follower);
        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                .WillOnce(Invoke(
                        [&](const auto& msg, auto)
                        {
                            auto root = *msg.get();
                            EXPECT_EQ(root["error"].asString(), ERROR_REMOVE_PEER_MUST_BE_SENT_TO_LEADER);
                        }));

        mh(make_remove_peer_request(),this->mock_session);

        // let's try a raft in candidate state, expire timer...
        // the current state must be follower
        // don't care about the handler...
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times(TEST_PEER_LIST.size() - 1);
        wh(boost::system::error_code());
        EXPECT_EQ(raft->get_state(), bzn::raft_state::candidate);

        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                .WillOnce(Invoke(
                        [&](const auto& msg, auto)
                        {
                            auto root = *msg.get();
                            EXPECT_EQ(root["error"].asString(), ERROR_REMOVE_PEER_MUST_BE_SENT_TO_LEADER);
                        }));

        mh(make_remove_peer_request(), this->mock_session);
    }


    TEST_F(raft_test, test_that_add_or_remove_peer_fails_if_current_quorum_is_a_joint_quorum)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        // craft raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        // and away we go...
        raft->start();

        // lets make this raft the leader by responding to requests for votes
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times((TEST_PEER_LIST.size() - 1) * 2);

        wh(boost::system::error_code());

        // send the votes
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), this->mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), this->mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        mh( make_add_peer_request(),this->mock_session);

        EXPECT_TRUE(raft->peer_match_index.find( new_peer.uuid ) != raft->peer_match_index.end());
        EXPECT_EQ(raft->peer_match_index[new_peer.uuid], size_t(1));

        // the end result will be the appending of a joint quorum to the log entries
        bzn::log_entry entry = raft->raft_log->last_quorum_entry();
        EXPECT_EQ(entry.entry_type, bzn::log_entry_type::joint_quorum);

        // Now that we have added a joint quorum, we should not be able to add or remove another peer
        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);
        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                .Times(2)
                .WillRepeatedly(Invoke(
                        [&](const auto& msg, auto)
                        {
                            auto root = *msg.get();
                            EXPECT_EQ(root["error"].asString(), MSG_ERROR_CURRENT_QUORUM_IS_JOINT);
                        }));

        mh( make_add_peer_request(),this->mock_session);
        mh( make_remove_peer_request(),this->mock_session);
    }



    TEST_F(raft_test, test_that_bad_add_or_remove_peer_requests_fail)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        // craft raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID,
                                                TEST_STATE_DIR);

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto &, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        // and away we go...
        raft->start();

        // lets make this raft the leader by responding to requests for votes
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times((TEST_PEER_LIST.size() - 1) * 2);

        wh(boost::system::error_code());

        // send the votes
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), this->mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), this->mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        //////
        // test add and remove peers with empty or missing UUID values will fail

        {
            bzn::message bad_add = make_add_peer_request();
            bad_add["data"]["peer"].removeMember("uuid");

            EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                    .Times(2)
                    .WillRepeatedly(Invoke(
                            [&](const auto& msg, auto)
                            {
                                auto root = *msg.get();
                                EXPECT_EQ(root["error"].asString(), ERROR_INVALID_UUID);
                            }));

            mh( bad_add,this->mock_session);
            bad_add["data"]["uuid"] = "";
            mh( bad_add,this->mock_session);
        }
        {


            EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                    .Times(3)
                    .WillRepeatedly(Invoke(
                            [&](const auto& msg, auto)
                            {
                                auto root = *msg.get();
                                EXPECT_EQ(root["error"].asString(), ERROR_INVALID_UUID);
                            }));

            bzn::message bad_remove = make_remove_peer_request();
            // msg["data"]["uuid"] = TEST_NODE_UUID;
            bad_remove.removeMember("data");
            mh( bad_remove,this->mock_session);
            bad_remove["data"] = bzn::message{};
            mh( bad_remove,this->mock_session);

            bad_remove["data"]["uuid"] = "";
            mh( bad_remove,this->mock_session);
        }
    }


    TEST_F(raft_test, test_that_add_peer_request_to_leader_results_in_correct_joint_quorum)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        // craft raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));


        bool commit_handler_called = false;
        int commit_handler_times_called = 0;
        raft->register_commit_handler(
                [&](const bzn::message& msg)
                {
                    LOG(info) << "commit:\n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

                    commit_handler_called = true;
                    ++commit_handler_times_called;
                    return true;
                });

        // and away we go...
        raft->start();

        // lets make this raft the leader by responding to requests for votes
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times((TEST_PEER_LIST.size() - 1) * 2);

        wh(boost::system::error_code());

        // send the votes
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), this->mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), this->mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        bzn::message msg = make_add_peer_request();
        mh(msg,this->mock_session);

        bzn::log_entry entry = raft->raft_log->last_quorum_entry();
        EXPECT_EQ(entry.entry_type, bzn::log_entry_type::joint_quorum);

        bzn::message jq{entry.msg["msg"]["peers"]};

        EXPECT_TRUE(jq.isMember("old"));
        EXPECT_TRUE(jq.isMember("new"));

        bzn::peers_list_t old_peers;
        bzn::peers_list_t new_peers;

        json_to_peers(jq["old"], old_peers);
        json_to_peers(jq["new"], new_peers);

        EXPECT_EQ(old_peers.size(), size_t(3));
        EXPECT_EQ(new_peers.size(), size_t(4));

        // ok the sizes are good, let's look at the contents.
        std::for_each(old_peers.begin(), old_peers.end(), [&](const auto& p) {
            const auto &r = std::find(new_peers.begin(), new_peers.end(), p);
            EXPECT_FALSE(r == new_peers.end());
        });

        // and new must also have the new peer
        EXPECT_FALSE(std::find(new_peers.begin(), new_peers.end(), new_peer) == new_peers.end());


        EXPECT_CALL(*this->mock_node, send_message(_, _)).WillRepeatedly(Invoke(
                [](const auto& /*session*/, const auto& /*msg*/)
                {
                    //std::cout << msg->toStyledString();
                }));


        // do the concensus and commit the joint quorum
        // expire timer...
        wh(boost::system::error_code());

        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid1", 1, true, 1), mock_session);
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid2", 1, true, 1), mock_session);
        raft->handle_request_append_entries_response(bzn::create_append_entries_response(TEST_NODE_UUID, 1, true, 1), mock_session);
        wh(boost::system::error_code());
}


    TEST_F(raft_test, test_that_remove_peer_request_to_leader_results_in_correct_joint_quorum)
    {
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        // craft raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        // and away we go...
        raft->start();

        // lets make this raft the leader by responding to requests for votes
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times((TEST_PEER_LIST.size() - 1) * 2);

        wh(boost::system::error_code());

        // send the votes
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), this->mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), this->mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        mh(make_remove_peer_request(), this->mock_session);

        // the end result will be the appending of a joint quorum to the log entries
        bzn::log_entry entry = raft->raft_log->last_quorum_entry();
        EXPECT_EQ(entry.entry_type, bzn::log_entry_type::joint_quorum);

        bzn::message jq{entry.msg["msg"]["peers"]};

        EXPECT_TRUE(jq.isMember("old"));
        EXPECT_TRUE(jq.isMember("new"));

        bzn::peers_list_t old_peers;
        bzn::peers_list_t new_peers;

        json_to_peers(jq["old"], old_peers);
        json_to_peers(jq["new"], new_peers);

        EXPECT_EQ(old_peers.size(), size_t(3));
        EXPECT_EQ(new_peers.size(), size_t(2));

        // ok the sizes are good, let's look at the contents.
        std::for_each(new_peers.cbegin(), new_peers.cend(), [&](const auto& p) {
            const auto &r = std::find(old_peers.cbegin(), old_peers.cend(), p);
            EXPECT_FALSE(r == old_peers.end());
        });

        const auto& should_be_end = std::find_if(new_peers.begin(), new_peers.end(), [&](const auto& p)
                {
                    return p.uuid == TEST_NODE_UUID;
                });

        EXPECT_TRUE(should_be_end == new_peers.end());
    }


    TEST_F(raft_test, test_that_add_peer_fails_when_the_peer_uuid_is_already_in_the_single_quorum)
    {
        // TODO : make sure add_peer fails when uuid is not unique
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        // craft raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        // and away we go...
        raft->start();

        // lets make this raft the leader by responding to requests for votes
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times((TEST_PEER_LIST.size() - 1) * 2);

        wh(boost::system::error_code());

        // send the votes
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), this->mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), this->mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                .WillOnce(Invoke(
                        [&](const auto& msg, auto)
                        {
                            auto root = *msg.get();
                            EXPECT_EQ(root["error"].asString(), ERROR_PEER_ALREADY_EXISTS);
                        }));

        mh(make_duplicate_add_peer_request(), this->mock_session);
    }


    TEST_F(raft_test, test_that_remove_peer_fails_when_the_peer_uuid_is_not_in_the_single_quorum)
    {
        // TODO : make sure add_peer fails when uuid is not unique
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        // craft raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        bzn::message_handler mh;
        EXPECT_CALL(*mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        // and away we go...
        raft->start();

        // lets make this raft the leader by responding to requests for votes
        EXPECT_CALL(*this->mock_node, send_message(_, _)).Times((TEST_PEER_LIST.size() - 1) * 2);

        wh(boost::system::error_code());

        // send the votes
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), this->mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), this->mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(),_))
                .WillOnce(Invoke(
                        [&](const auto& msg, auto)
                        {
                            auto root = *msg.get();
                            EXPECT_EQ(root["error"].asString(), ERROR_PEER_NOT_FOUND);
                        }));

        mh(make_remove_nonexisting_peer_request(), this->mock_session);
    }


    TEST(raft, test_that_get_all_peers_returns_correct_peers_list_based_on_current_quorum)
    {
        clean_state_folder();
        auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);
        auto peers = raft.get_all_peers();

        EXPECT_EQ(TEST_PEER_LIST.size(), peers.size());
        EXPECT_EQ(TEST_PEER_LIST,peers);

        // replace the last quorum with a joint quorum
        raft.current_state = bzn::raft_state::leader;
        raft.handle_ws_raft_messages(make_add_peer_request(), nullptr);

        const auto& active = raft.get_active_quorum();
        EXPECT_EQ(active.size(), size_t(2));

        std::for_each(TEST_PEER_LIST.begin()
                , TEST_PEER_LIST.end()
                , [&](const auto& peer)
                      {
                          for(const auto& quorum_uuids : active)
                          {
                              const auto& found = std::find_if(
                                      quorum_uuids.begin()
                                      , quorum_uuids.end()
                                      ,[&](const auto& uuid)
                                      {
                                          return peer.uuid == uuid;
                                      });
                              EXPECT_TRUE(found != quorum_uuids.end());
                          }
                      });
        const auto& new_set = active.back();
        EXPECT_EQ(new_set.size(), size_t(4));
        const auto& new_peer_uuid = std::find_if(new_set.begin(), new_set.end(),[&](const auto& u)
                {
                    return u == new_peer.uuid;
                });
        EXPECT_EQ(*new_peer_uuid, new_peer.uuid);

        peers = raft.get_all_peers();
        EXPECT_EQ(peers.size(), TEST_PEER_LIST.size() + 1);

        clean_state_folder();
    }


    TEST(raft, test_get_active_quorum_returns_single_or_joint_quorum_appropriately)
    {
        clean_state_folder();
        auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        EXPECT_EQ(bzn::log_entry_type::single_quorum, raft.raft_log->last_quorum_entry().entry_type);

        auto active_list = raft.get_active_quorum();

        EXPECT_EQ(active_list.size(), size_t(1));

        std::set<bzn::uuid_t> peers = active_list.front();
        std::set<bzn::uuid_t> expected_peers;
        std::transform(TEST_PEER_LIST.begin(), TEST_PEER_LIST.end(), std::inserter(expected_peers, expected_peers.begin())
                , [](const auto& peer)
                       {
                           return peer.uuid;
                       });

        std::set<bzn::uuid_t> peer_set;
        std::set_intersection(
                expected_peers.begin()
                , expected_peers.end()
                , peers.begin()
                , peers.end()
                , std::inserter(peer_set,peer_set.begin()));

        EXPECT_EQ(peer_set.size(), TEST_PEER_LIST.size());

        // replace the last quorum with a joint quorum
        bzn::message add_peer = make_add_peer_request();
        raft.current_state = bzn::raft_state::leader;
        raft.handle_ws_raft_messages(add_peer, nullptr);

        active_list = raft.get_active_quorum();
        EXPECT_EQ(active_list.size(), size_t(2));
        const auto old_peers = active_list.front();
        const auto new_peers = active_list.back();

        EXPECT_EQ(old_peers.size(), size_t(3));
        EXPECT_EQ(new_peers.size(), size_t(4));

        peer_set.clear();


        std::set_difference(new_peers.begin(), new_peers.end()
                , old_peers.begin(), old_peers.end()
                , std::inserter(peer_set,peer_set.begin()));

        EXPECT_EQ(peer_set.size(), size_t(1));

        EXPECT_EQ(new_peer.uuid, *peer_set.find(new_peer.uuid));

        clean_state_folder();
    }


    TEST(raft, test_that_is_majority_returns_expected_result_for_single_and_joint_quorums)
    {
        clean_state_folder();
        auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        EXPECT_EQ(bzn::log_entry_type::single_quorum, raft.raft_log->last_quorum_entry().entry_type);

        std::set<bzn::uuid_t> yes_votes;
        auto peer_iter = TEST_PEER_LIST.begin();
        yes_votes.emplace(peer_iter->uuid);
        EXPECT_FALSE(raft.is_majority(yes_votes));

        ++peer_iter;
        yes_votes.emplace(peer_iter->uuid);
        EXPECT_TRUE(raft.is_majority(yes_votes));

        ++peer_iter;
        yes_votes.emplace(peer_iter->uuid);
        EXPECT_TRUE(raft.is_majority(yes_votes));

        raft.current_state = bzn::raft_state::leader;
        raft.handle_ws_raft_messages(make_add_peer_request(), nullptr);

        yes_votes.clear();
        yes_votes.emplace(new_peer.uuid);
        EXPECT_FALSE(raft.is_majority(yes_votes));

        peer_iter = TEST_PEER_LIST.begin();
        yes_votes.emplace(peer_iter->uuid);
        EXPECT_FALSE(raft.is_majority(yes_votes));
        clean_state_folder();
    }


    TEST_F(raft_test, test_that_joint_quorum_is_converted_to_single_quorum_and_committed)
    {
        // TODO make sure comit i=indexes are correct.
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

        // intercept the timeout callback...
        bzn::asio::wait_handler wh;
        EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
                [&](auto handler)
                { wh = handler; }));

        EXPECT_CALL(*this->mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
                [&]()
                { return std::move(mock_steady_timer); }));

        bzn::message_handler mh;
        EXPECT_CALL(*this->mock_node, register_for_message("raft", _)).WillOnce(Invoke(
                [&](const auto&, auto handler)
                {
                    mh = handler;
                    return true;
                }));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID, TEST_STATE_DIR);

        bool commit_handler_called = false;
        int commit_handler_times_called = 0;
        raft->register_commit_handler(
                [&](const bzn::message& msg)
        {
                    LOG(info) << "commit:\n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

            commit_handler_called = true;
            ++commit_handler_times_called;
            return true;
                });

        // and away we go...
        raft->start();

        // get the raft into the leader state.
        // expire election timer...

        // we should see requests...
        EXPECT_CALL(*mock_node, send_message(_, _)).WillRepeatedly(
                Invoke([](const auto&, const auto& /*msg*/){

                })
        );

        wh(boost::system::error_code());

        EXPECT_EQ(raft->get_state(), bzn::raft_state::candidate);

        // now send in each vote...
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid1", 1, true), mock_session);
        raft->handle_request_vote_response(bzn::create_request_vote_response("uuid2", 1, true), mock_session);

        EXPECT_EQ(raft->get_state(), bzn::raft_state::leader);

        // add a peer
        bzn::message msg = make_add_peer_request();
        mh(msg,this->mock_session);

        bzn::log_entry entry = raft->raft_log->last_quorum_entry();
        EXPECT_EQ(entry.entry_type, bzn::log_entry_type::joint_quorum);

        // next heartbeat...
        wh(boost::system::error_code());

        // send false so second peer will achieve consensus and leader will commit the entries..
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid1", 1, false, 1), this->mock_session);

        EXPECT_EQ(commit_handler_times_called, 0);
        ASSERT_FALSE(commit_handler_called);

        // enough peers have stored the first entry
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid1", 1, true, 2), this->mock_session);
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid2", 1, true, 2), this->mock_session);
        raft->handle_request_append_entries_response(bzn::create_append_entries_response(TEST_NODE_UUID, 1, true, 2), this->mock_session);
        
        EXPECT_EQ(commit_handler_times_called, 1);

        EXPECT_EQ((size_t)3, raft->raft_log->size());

        // expire heart beat
        wh(boost::system::error_code());

        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid1", 1, true, 3), this->mock_session);
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid2", 1, true, 3), this->mock_session);
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid_new", 1, true, 3), this->mock_session);
        raft->handle_request_append_entries_response(bzn::create_append_entries_response(TEST_NODE_UUID, 1, true, 3), this->mock_session);
        EXPECT_EQ(commit_handler_times_called, 2);

        entry = raft->raft_log->last_quorum_entry();
        EXPECT_EQ(entry.entry_type, bzn::log_entry_type::single_quorum);
    }


    TEST(raft, test_log_entry_type_to_string)
    {
        EXPECT_EQ(bzn::LOG_ENTRY_TYPES[0], bzn::log_entry_type_to_string(bzn::log_entry_type::database));
        EXPECT_EQ(bzn::LOG_ENTRY_TYPES[1], bzn::log_entry_type_to_string(bzn::log_entry_type::single_quorum));
        EXPECT_EQ(bzn::LOG_ENTRY_TYPES[2], bzn::log_entry_type_to_string(bzn::log_entry_type::joint_quorum));
        EXPECT_EQ(bzn::LOG_ENTRY_TYPES[3], bzn::log_entry_type_to_string(bzn::log_entry_type::undefined));
    }
} // bzn
