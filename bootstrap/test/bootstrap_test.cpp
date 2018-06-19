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

#include <bootstrap/bootstrap_peers.hpp>
#include <bootstrap/bootstrap_peers_base.hpp>
#include <fstream>
#include <string>
#include <gtest/gtest.h>

namespace
{
    const std::string invalid_json = "[{\"Some key\": 124}, }}[} oh noes I broke it";
    const std::string no_peers = "[]";
    const std::string valid_peers = "[{\"name\": \"peer1\", \"host\": \"peer1.com\", \"port\": 12345, \"http_port\" : 8080}, {\"host\": \"nonamepeer.com\", \"port\": 54321, \"http_port\" : 8080}]";
    const std::string duplicate_peers = "[{\"name\": \"peer1\", \"host\": \"peer1.com\", \"port\": 12345, \"http_port\" : 8080}, {\"name\": \"peer1\", \"host\": \"peer1.com\", \"port\": 12345, \"http_port\" : 8080}]";
    const std::string underspecified_peer = "[{\"name\": \"peer1\", \"port\": 1024}]";
    const std::string bad_port = "[{\"name\": \"peer1\", \"host\": \"127.0.0.1\", \"port\": 70000}]";
    const std::string test_peers_filename = "peers.json";

    const std::string sample_peers_url = "pastebin.com/raw/KvbcVhfZ";
    const std::string sample_peers_url_with_protocol = "http://www.pastebin.com/raw/KvbcVhfZ";
}

using namespace ::testing;


class bootstrap_file_test : public Test
{
public:
    bzn::bootstrap_peers bootstrap_peers;

    void set_peers_data(const std::string& peers_data)
    {
        std::ofstream ofile(test_peers_filename);
        ofile << peers_data;
        ofile.close();
    }

    ~bootstrap_file_test()
    {
        unlink(test_peers_filename.c_str());
    }
};


TEST_F(bootstrap_file_test, test_invalid_json)
{
    set_peers_data(invalid_json);
    ASSERT_FALSE(bootstrap_peers.fetch_peers_from_file(test_peers_filename));
    ASSERT_TRUE(bootstrap_peers.get_peers().empty());
}


TEST_F(bootstrap_file_test, test_no_peers)
{
    set_peers_data(no_peers);
    ASSERT_FALSE(bootstrap_peers.fetch_peers_from_file(test_peers_filename));
    ASSERT_TRUE(bootstrap_peers.get_peers().empty());
}


TEST_F(bootstrap_file_test, test_underspecified_peer)
{
    set_peers_data(underspecified_peer);
    ASSERT_FALSE(bootstrap_peers.fetch_peers_from_file(test_peers_filename));
    ASSERT_TRUE(bootstrap_peers.get_peers().empty());
}


TEST_F(bootstrap_file_test, test_bad_port)
{
    set_peers_data(bad_port);
    ASSERT_FALSE(bootstrap_peers.fetch_peers_from_file(test_peers_filename));
    ASSERT_TRUE(bootstrap_peers.get_peers().empty());
}


TEST_F(bootstrap_file_test, test_valid_peers)
{
    set_peers_data(valid_peers);
    ASSERT_TRUE(bootstrap_peers.fetch_peers_from_file(test_peers_filename));
    ASSERT_EQ(bootstrap_peers.get_peers().size(), 2U);

    bool seen_peer1 = false;
    bool seen_peer2 = false;

    for (const bzn::peer_address_t& p : bootstrap_peers.get_peers())
    {
        if(p.port == 12345) seen_peer1 = true;
        if(p.port == 54321) seen_peer2 = true;
    }

    ASSERT_TRUE(seen_peer1 && seen_peer2);
}


TEST_F(bootstrap_file_test, test_unnamed_peers)
{
    set_peers_data(valid_peers);
    ASSERT_TRUE(bootstrap_peers.fetch_peers_from_file(test_peers_filename));
    ASSERT_EQ(bootstrap_peers.get_peers().size(), 2U);

    bool seen_name1 = false;
    bool seen_name2 = false;

    for (const bzn::peer_address_t& p : bootstrap_peers.get_peers())
    {
        if(p.name == "peer1") seen_name1 = true;
        if(p.name == "unknown") seen_name2 = true;
    }

    ASSERT_TRUE(seen_name1 && seen_name2);
}


TEST_F(bootstrap_file_test, test_duplicate_peers)
{
    set_peers_data(duplicate_peers);
    ASSERT_TRUE(bootstrap_peers.fetch_peers_from_file(test_peers_filename));
    ASSERT_EQ(bootstrap_peers.get_peers().size(), 1U);
}


TEST(bootstrap_net_test, DISABLED_test_fetch_data)
{
    bzn::bootstrap_peers bootstrap_peers;
    // I don't like this test dependancy, but mocking out the http stuff
    // would mean mocking out all the meaningful code that this touches
    ASSERT_TRUE(bootstrap_peers.fetch_peers_from_url(sample_peers_url));
    ASSERT_EQ(bootstrap_peers.get_peers().size(), 1U);
}


TEST(bootstrap_net_test, DISABLED_test_fetch_data_with_protocol)
{
    bzn::bootstrap_peers bootstrap_peers;
    ASSERT_TRUE(bootstrap_peers.fetch_peers_from_url(sample_peers_url_with_protocol));
    ASSERT_EQ(bootstrap_peers.get_peers().size(), 1U);
}
