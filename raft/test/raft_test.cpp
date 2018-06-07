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
#include <storage/storage.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <random>
#include <stdlib.h>

using namespace ::testing;

namespace
{
    const bzn::uuid_t TEST_NODE_UUID{"f0645cc2-476b-485d-b589-217be3ca87d5"};

    const bzn::peers_list_t TEST_PEER_LIST{{"127.0.0.1", 8081, "name1", "uuid1"},
                                           {"127.0.0.1", 8082, "name2", "uuid2"},
                                           {"127.0.0.1", 8084, "name3", TEST_NODE_UUID}};

    const std::string RAFT_TIMEOUT_SCALE = "RAFT_TIMEOUT_SCALE";


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
                          entry.entry_type = bzn::log_entry_type::log_entry;
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
            if(boost::filesystem::exists("./.state"))
            {
                boost::filesystem::remove_all("./.state");
            }
            boost::filesystem::create_directory("./.state");
        }
        catch(boost::filesystem::filesystem_error const & e)
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
        EXPECT_EQ(bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
            nullptr, TEST_PEER_LIST, TEST_NODE_UUID).get_state(), bzn::raft_state::follower);
        clean_state_folder();
    }


    TEST(raft, test_that_a_raft_constructor_throws_when_given_empty_peers_list)
    {
        EXPECT_THROW(bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
            nullptr, {}, TEST_NODE_UUID), std::runtime_error);

        EXPECT_NO_THROW(bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
            nullptr, TEST_PEER_LIST, TEST_NODE_UUID));
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

        EXPECT_CALL(*this->mock_node, register_for_message("raft", _));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

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
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

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
        EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<bzn::message>>(), _)).WillOnce(Invoke(
            [&](const auto& msg, auto)
            { resp = *msg; }));

        // send a message through the registered "node message" callback...
        mh(bzn::create_request_vote_request("uuid1", 2, 0, 0), mock_session);

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
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

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

        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

        raft->start();

        bzn::message resp;
        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(), _)).Times(1).WillRepeatedly(Invoke(
            [&](const auto& msg, auto)
            { resp = *msg; }));

        // send a message through the registered "node message" callback...
        bzn::message msg;
        mh(bzn::create_append_entries_request("uuid", 1, 0, 0, 0, 0, msg), mock_session);

        // we expect a "no" response in this state...
        EXPECT_EQ(resp["cmd"].asString(), "AppendEntriesReply");
        EXPECT_EQ(resp["data"]["success"].asBool(), true);
    }


    TEST_F(raft_test, test_that_leader_sends_entries_and_commits_when_enough_peers_have_saved_them)
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

        EXPECT_CALL(*this->mock_node, register_for_message("raft", _));

        // create raft...
        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, TEST_NODE_UUID);

        // and away we go...
        raft->start();

        EXPECT_EQ(raft->get_leader().uuid, "");

        // try to append a log. It will fail since we are not the leader...
        ASSERT_FALSE(raft->append_log(bzn::message()));

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

        ///////////////////////////////////////////////////////////////////////////

        bool commit_handler_called = false;
        int commit_handler_times_called = 0;
        raft->register_commit_handler(
            [&](const bzn::message& msg)
            {
                LOG(info) << "commit:\n" << msg.toStyledString().substr(0, 60) << "...";

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

        // next heartbeat...
        wh(boost::system::error_code());

        // send false so second peer will achieve consensus and leader will commit the entries..
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid1", 2, false, 0), mock_session);

        EXPECT_EQ(commit_handler_times_called, 0);
        ASSERT_FALSE(commit_handler_called);

        // enough peers have stored the first entry
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid2", 2, true, 1), this->mock_session);

        EXPECT_EQ(commit_handler_times_called, 1);

        // expire heart beat
        wh(boost::system::error_code());

        // enough peers have stored the first entry
        commit_handler_times_called = 0;
        commit_handler_called = false;
        raft->handle_request_append_entries_response(bzn::create_append_entries_response("uuid2", 2, true, 2), this->mock_session);

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

        auto raft = std::make_shared<bzn::raft>(this->mock_io_context, this->mock_node, TEST_PEER_LIST, "uuid1");

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
        EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<bzn::message>>(), _)).WillRepeatedly(Invoke(
            [&](const auto& msg, auto /*handler*/)
            {
                resp = *msg;
            }));

        int commit_handler_times_called = 0;
        raft->register_commit_handler(
            [&](const bzn::message& msg)
            {
                LOG(info) << "commit:\n" << msg.toStyledString().substr(0, 60) << "...";
                ++commit_handler_times_called;
                return true;
            });

        ///////////////////////////////////////////////////////////////////////////
        bzn::message entry;
        entry["bzn-api"] = "utest";

        auto msg = bzn::create_append_entries_request(TEST_NODE_UUID, 2, 0, 0, 0, 0, bzn::message());
        mh(msg, this->mock_session);
        ASSERT_TRUE(resp["data"]["success"].asBool());

        resp.clear();

        // send append entry with commit index of 1... and follower commits and updates its match index
        msg = bzn::create_append_entries_request(TEST_NODE_UUID, 2, 1, 0, 0, 2, entry);
        mh(msg, this->mock_session);
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
        mh(msg, this->mock_session);
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


    TEST(raft, test_log_entry_serialization)
    {
        const size_t number_of_entries = 3;
        const std::string path{"./.state/test_data.dat"};
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


    TEST(raft, test_that_raft_can_rehydrate_state_and_log_entries)
    {
        clean_state_folder();

        auto mock_session = std::make_shared<bzn::Mocksession_base>();
        auto raft_source = std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID);

        const size_t number_of_entries = 300;

        fill_entries_with_test_data(number_of_entries, raft_source->log_entries);

        // TODO: this should be done via RAFT, not by cheating. That is, I'd like to simulate the append entry process to ensure that append_entry_to_log gets called
        std::for_each(raft_source->log_entries.begin(), raft_source->log_entries.end(),
                      [&](const auto &log_entry)
                      {
                          if (log_entry.log_index > 0)
                          {
                              raft_source->append_entry_to_log(log_entry);
                          }
                      });

        raft_source->last_log_index = raft_source->log_entries.back().log_index;
        raft_source->last_log_term = raft_source->log_entries.back().term;
        raft_source->commit_index = raft_source->log_entries.back().log_index;
        raft_source->current_term = raft_source->last_log_term;

        // TODO: This should be done via RAFT, not by cheating.
        raft_source->save_state();

        // instantiate a raft with the same uuid
        auto raft_target = std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID);

        EXPECT_EQ(raft_target->last_log_index, raft_source->last_log_index);
        EXPECT_EQ(raft_target->last_log_term, raft_source->last_log_term);
        EXPECT_EQ(raft_target->commit_index, raft_source->commit_index);
        EXPECT_EQ(raft_target->current_term, raft_source->current_term);

        auto source_entries = raft_source->log_entries;
        auto target_entries = raft_target->log_entries;

        EXPECT_TRUE(target_entries.size()>0);
        EXPECT_EQ(target_entries.size(),source_entries.size());

        EXPECT_TRUE(std::equal(
                std::begin(target_entries), std::end(target_entries),
                std::begin(source_entries),
                [](const bzn::log_entry &rhs, const bzn::log_entry &lhs) -> bool
                {
                    return (rhs.log_index == lhs.log_index
                            && rhs.term == lhs.term
                            && rhs.msg.toStyledString() == lhs.msg.toStyledString()
                    );
                }
        ));

        clean_state_folder();
    }

    
    TEST(raft, test_that_raft_can_rehydrate_storage)
    {
        const size_t number_of_entries = 300;

        clean_state_folder();

        auto mock_session = std::make_shared<bzn::Mocksession_base>();
        auto raft_source = std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(),
                                                       nullptr, TEST_PEER_LIST, TEST_NODE_UUID);
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
                raft_source->append_log(message);
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
                    raft_source->append_log(message);
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
                    raft_source->append_log(message);
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
        const std::string state_path{"./.state/" + TEST_NODE_UUID + ".state"};
        const std::string log_path{"./.state/" + TEST_NODE_UUID + ".dat"};

        const std::string good_state{"1 0 1 4"};
        const std::string bad_state{"X 0 X X"};

        const std::string bad_entry_00{"1 4 THIS_IS_BADeyJiem4tYXB0K"};
        const std::string valid_entry{"1 2 eyJiem4tYXBpIjoiY3J1ZCIsImNtZCI6ImNyZWF0ZSIsImRhdGEiOnsia2V5Ijoia2V5MCIsInZhbHVlIjoidmFsdWVfZm9yX2tleTAifSwiZGItdXVpZCI6Im15LXV1aWQiLCJyZXF1ZXN0LWlkIjowfQo="};

        clean_state_folder();

        // good state/ empty entry
        std::ofstream out(state_path, std::ios::out | std::ios::binary);
        out << good_state;
        out.close();

        out.open(log_path, std::ios::out | std::ios::binary);
        out << "";
        out.close();

        EXPECT_THROW(
                std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID)
                        , std::runtime_error);

        // good state/bad entry
        out.open(log_path, std::ios::out | std::ios::binary);
        out << bad_entry_00;
        out.close();

        EXPECT_THROW(
                std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID)
                        , std::runtime_error);

        // good state/good entry/mis-matched
        out.open(log_path, std::ios::out | std::ios::binary);
        out << valid_entry;
        out.close();

        EXPECT_THROW(
                std::make_shared<bzn::raft>(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID)
                        , std::runtime_error);

        clean_state_folder();
    }


    TEST(raft, test_raft_can_find_last_quorum_log_entry)
    {
        auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID);

        bzn::message msg;

        msg["data"] = "data";

        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::single_quorum, 1, 1, msg});

        auto quorum = raft.last_quorum();
        EXPECT_EQ(quorum.entry_type, bzn::log_entry_type::single_quorum);
        EXPECT_EQ((uint32_t)1, quorum.log_index);
        EXPECT_EQ((uint32_t)1, quorum.term);

        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::log_entry, 2, 2, msg});
        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::log_entry, 3, 3, msg});
        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::log_entry, 4, 5, msg});
        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::log_entry, 5, 8, msg});

        quorum = raft.last_quorum();
        EXPECT_EQ(quorum.entry_type, bzn::log_entry_type::single_quorum);
        EXPECT_EQ((uint32_t)1, quorum.log_index);
        EXPECT_EQ((uint32_t)1, quorum.term);

        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::joint_quorum, 6, 13, msg});
        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::log_entry, 7, 21, msg});
        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::log_entry, 8, 34, msg});
        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::log_entry, 9, 55, msg});

        quorum = raft.last_quorum();
        EXPECT_EQ(quorum.entry_type, bzn::log_entry_type::joint_quorum);
        EXPECT_EQ((uint32_t)6, quorum.log_index);
        EXPECT_EQ((uint32_t)13, quorum.term);

        raft.log_entries.emplace_back(log_entry{bzn::log_entry_type::joint_quorum, 10, 89, msg});

        quorum = raft.last_quorum();
        EXPECT_EQ(quorum.entry_type, bzn::log_entry_type::joint_quorum);
        EXPECT_EQ((uint32_t)10, quorum.log_index);
        EXPECT_EQ((uint32_t)89, quorum.term);
    }

    TEST_F(raft_test, test_that_raft_first_log_entry_is_the_quorum)
    {
        clean_state_folder();


        bzn::message expected;
        auto raft = bzn::raft(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>(), nullptr, TEST_PEER_LIST, TEST_NODE_UUID);
        EXPECT_EQ(raft.log_entries.size(), static_cast<size_t>(1));
        EXPECT_EQ(raft.log_entries.front().entry_type, bzn::log_entry_type::single_quorum);
        bzn::message msg = raft.log_entries.front().msg;
        
        for (const auto& p : msg)
        {
            bzn::peer_address_t peer_address(p["host"].asString(), (uint16_t)p["port"].asUInt(), p["name"].asString(), p["uuid"].asString());
            EXPECT_TRUE(TEST_PEER_LIST.find(peer_address) != TEST_PEER_LIST.end());
        }

        clean_state_folder();
    }

} // bzn
