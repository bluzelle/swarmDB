// Copyright (c) 2017-2018 Bluzelle Networks
//
// This file is part of Bluzelle.
//
// Bluzelle is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <include/bluzelle.hpp>
#include <options/options.hpp>
#include <fstream>
#include <gtest/gtest.h>


namespace
{
    const char* NO_ARGS[] = {"config_tests"};

    const char* VALID_ARGS[] = {"config_tests", "-l", "0.0.0.0", "-p", "50000",  "-a", "0x006eae72077449caca91078ef78552c0cd9bce8f"};

    const char* VALID_ARGS_WITH_PEERS[] = {
        "config_tests", "-l", "0.0.0.0", "-p", "50000",  "-a", "0x006eae72077449caca91078ef78552c0cd9bce8f",
        "-b", "peers_from_cmd.json", "--bootstrap_url", "example.org/peers_from_cmd.json"};

    const char* INVALID_port_ARGS[] = {"config_tests",  "-l",  "0.0.0.0", "-p", "90000", "-a", "0x006eae72077449caca91078ef78552c0cd9bce8f"};

    const char* INVALID_ethererum_ARGS[] = {"config_tests",  "-l",  "0.0.0.0", "-p", "50000", "-a", "0x006eae72077449Zcaca91078ef78552c0cd9bce8f"};

    const char* INVALID_listener_ARGS[] = {"config_tests", "-p", "50000", "-a", "0x006eae72077449caca91078ef78552c0cd9bce8f"};

    const std::string TEST_CONFIG_FILE = "bluzelle.json";

    const std::string DEFAULT_CONFIG_DATA = "{\n"
        "  \"listener_address\" : \"0.0.0.0\",\n"
        "  \"listener_port\" : 49152,\n"
        "  \"ethereum\" : \"0x006eae72077449caca91078ef78552c0cd9bce8f\",\n"
        "  \"bootstrap_file\" : \"peers.json\",\n"
        "  \"bootstrap_url\"  : \"example.org/peers.json\"\n"
        "}";

    const auto DEFAULT_LISTENER = boost::asio::ip::tcp::endpoint{boost::asio::ip::address::from_string("0.0.0.0"), 49152};
}

using namespace ::testing;


// create default options and remove when done...
class options_file_test : public Test
{
public:
    options_file_test()
    {

        std::ofstream ofile(TEST_CONFIG_FILE);
        ofile.exceptions(std::ios::failbit);
        ofile << DEFAULT_CONFIG_DATA;
    }

    ~options_file_test()
    {
        unlink(TEST_CONFIG_FILE.c_str());
    }
};


TEST_F(options_file_test, test_that_loading_of_default_config_file)
{
    bzn::options options;
    options.parse_command_line(1, NO_ARGS);

    EXPECT_EQ("0x006eae72077449caca91078ef78552c0cd9bce8f", options.get_ethererum_address());
    EXPECT_EQ(DEFAULT_LISTENER, options.get_listener());
    //EXPECT_EQ("peers.json", options.get_bootstrap_peers_file());
    //EXPECT_EQ("example.org/peers.json", options.get_bootstrap_peers_url());
}


TEST(options, test_that_missing_default_config_throws_exception)
{
    bzn::options options;

    EXPECT_THROW(options.parse_command_line(1, NO_ARGS), std::runtime_error);
}


TEST(options, test_that_all_valid_invalid_params)
{
    {
        bzn::options options;
        ASSERT_TRUE(options.parse_command_line(7, VALID_ARGS));
    }

    {
        bzn::options options;
        ASSERT_FALSE(options.parse_command_line(7, INVALID_port_ARGS));
    }

    {
        bzn::options options;
        ASSERT_FALSE(options.parse_command_line(7, INVALID_ethererum_ARGS));
    }

    {
        bzn::options options;
        ASSERT_FALSE(options.parse_command_line(5, INVALID_listener_ARGS));
    }
}

TEST(options, test_that_bootstrap_config_read_from_cmd)
{
    {
        bzn::options options;
        ASSERT_TRUE(options.parse_command_line(11, VALID_ARGS_WITH_PEERS));
        ASSERT_EQ("peers_from_cmd.json", options.get_bootstrap_peers_file());
        ASSERT_EQ("example.org/peers_from_cmd.json", options.get_bootstrap_peers_url());
    }
}
