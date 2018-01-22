#include "Daemon.h"

#include <boost/test/unit_test.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

const boost::regex ms_guid_expr{"[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}"};
const boost::regex udid_expr{"0x[a-fA-F0-9]{40}"};
const boost::regex blz_token_expr{"[0-9]* BLZ"};

const std::pair<std::string, std::string> parse_line(const std::string& line);

struct F
{
    F() = default;
    ~F() = default;
};


BOOST_FIXTURE_TEST_SUITE(daemon_suite, F)

    BOOST_AUTO_TEST_CASE(test_init)
    {
        Daemon sut0;
        auto node_id0 = sut0.node_id();
        BOOST_CHECK(
            true == boost::regex_match(
                node_id0,
                ms_guid_expr));

        Daemon sut1;
        BOOST_CHECK(
            true == boost::regex_match(
                sut1.node_id(),
                ms_guid_expr));

        BOOST_CHECK( node_id0 != sut1.node_id());
    }


    BOOST_AUTO_TEST_CASE( test_get_ethereum_adress )
    {
        const std::string  accepted_address{"0x106aaf72087449caca91078e4b8552c0cd9bde8a"};
        DaemonInfo::get_instance().ethereum_address() = accepted_address;
        Daemon sut;
        BOOST_CHECK(sut.ethereum_address()==accepted_address);
    }


    BOOST_AUTO_TEST_CASE(test_info)
    {
        const std::string  accepted_address{"0x106aaf72087449caca91078e4b8552c0cd9bde8a"};
        DaemonInfo::get_instance().ethereum_address() = accepted_address;
        const uint16_t accepted_port = 58001;
        DaemonInfo::get_instance().host_port() = accepted_port;

        Daemon sut;
        std::istringstream ss(sut.get_info());
        std::string line;

        bool has_node_id = false;
        bool has_ethereum_addr_id = false;
        bool has_port = false;
        bool has_token_balance = false;

        while(std::getline(ss, line))
            {
            const std::pair<std::string, std::string> p = parse_line(line);

            has_node_id = ((p.first == "Running node with ID") && boost::regex_match( p.second, ms_guid_expr)) || has_node_id;

            has_ethereum_addr_id =  (( p.first == "Ethereum Address ID") && boost::regex_match( p.second, udid_expr) ) || has_ethereum_addr_id;

            has_port = (( p.first == "on port") && (accepted_port == boost::lexical_cast<uint16_t >(p.second))) || has_port;

            has_token_balance = (( p.first == "Token Balance") && boost::regex_match(p.second, blz_token_expr)) || has_token_balance;
            }

        BOOST_CHECK(has_node_id);
        BOOST_CHECK(has_ethereum_addr_id);
        BOOST_CHECK(has_port);
        BOOST_CHECK(has_token_balance);
    }

BOOST_AUTO_TEST_SUITE_END()

const std::pair<std::string, std::string> parse_line(const std::string& line)
{
    const auto colon_pos = line.find(':');
    return std::make_pair(
        boost::trim_copy(line.substr(0,colon_pos)),
        boost::trim_copy(line.substr(colon_pos+1)));
};
