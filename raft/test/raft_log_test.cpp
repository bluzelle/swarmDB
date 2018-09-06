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
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/filesystem/operations.hpp>

using namespace ::testing;

namespace
{
    boost::random::mt19937 gen;

    const bzn::uuid_t TEST_NODE_UUID{"f0645cc2-476b-485d-b589-217be3ca87d5"};
    const bzn::peers_list_t TEST_PEER_LIST{{"127.0.0.1", 8081, 80, "name1", "uuid1"},
                                           {"127.0.0.1", 8082, 81, "name2", "uuid2"},
                                           {"127.0.0.1", 8084, 82, "name3", TEST_NODE_UUID}};

    std::string
    generate_test_string(int n = 100)
    {
        std::string test_data;
        test_data.resize(n);

        boost::random::uniform_int_distribution<> dist('a', 'z');
        for(auto& c : test_data)
        {
            c = (char)dist(gen);
        }
        return test_data;
    }

    bzn::message
    generate_test_message()
    {
        bzn::message msg;
        msg["bzn-api"] = "test";
        msg["cmd"] = "test";
        msg["data"] = generate_test_string(40);
        return msg;
    }


    void
    create_initial_entries_log(const std::string& path)
    {
        std::ofstream out(path, std::ios::out | std::ios::binary);
        out << bzn::log_entry{bzn::log_entry_type::single_quorum, 0, 0, bzn::message{}};

        for(uint32_t i=1;i<10;++i)
        {
            out << bzn::log_entry{bzn::log_entry_type::database, i, 1, generate_test_message()};
        }
        out.close();
    }

    void
    create_state_file(const std::string& path, size_t term, size_t index)
    {
        std::ofstream os( path ,std::ios::out | std::ios::binary);
        os << term << " " << index << " " <<  term;
        os.close();
    }


    void
    remove_folder(const std::string& path)
    {
        if (boost::filesystem::exists(path))
        {
            boost::filesystem::remove_all(path);
        }
    }
}


namespace bzn
{
    TEST(raft_log, test_that_raft_log_tracks_storage_used)
    {
        const std::string test_path{"./raft_log_test.dat"};
        unlink(test_path.c_str());

        create_initial_entries_log(test_path);

        bzn::raft_log sut(test_path);
        EXPECT_EQ(size_t(boost::filesystem::file_size(test_path)), sut.memory_used());

        for (uint32_t i = 1; i < 100; ++i)
        {
            sut.leader_append_entry(bzn::log_entry{bzn::log_entry_type::database, i, 1, generate_test_message()});
        }
        EXPECT_EQ(size_t(boost::filesystem::file_size(test_path)), sut.memory_used());

        unlink(test_path.c_str());
    }

    TEST(raft_log, test_that_raft_throws_on_start_when_max_storage_is_exceeded)
    {
        const size_t MAX_STORAGE_BYTES = 1000;
        const std::string TEST_STATE_DIR = "./.raft_test_state";
        const std::string ENTRIES_LOG = TEST_STATE_DIR + "/" + TEST_NODE_UUID + ".dat";
        const std::string STATE_LOG = TEST_STATE_DIR + "/" + TEST_NODE_UUID + ".state";
        remove_folder(TEST_STATE_DIR);

        boost::filesystem::create_directory(TEST_STATE_DIR);


        create_initial_entries_log(ENTRIES_LOG);
        create_state_file(STATE_LOG, 1, 9);

        try
        {
            bzn::raft(
                    std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>()
                    , nullptr
                    , TEST_PEER_LIST
                    , TEST_NODE_UUID
                    , TEST_STATE_DIR
                    , MAX_STORAGE_BYTES);
            FAIL() << "raft with entry log size exceeding max storage size should have thrown an exception";
        }
        catch(const std::runtime_error& e)
        {
            EXPECT_EQ(e.what(), bzn::MSG_ERROR_MAXIMUM_STORAGE_EXCEEDED);
        }
        remove_folder(TEST_STATE_DIR);
    }
}