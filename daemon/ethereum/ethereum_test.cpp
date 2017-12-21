#define BOOST_TEST_DYN_LINK

#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>

#include "EthereumApi.h"

constexpr char s_etherscan_api_token_envar_name[] = "ETHERSCAN_IO_API_TOKEN";


// --run_test=test_token_balance
BOOST_AUTO_TEST_CASE( test_token_balance ) {
    try
        {
        boost::asio::io_service ios;

        std::string token = getenv(s_etherscan_api_token_envar_name);
        BOOST_CHECK(!token.empty());

        EthereumApi api("0x006eae72077449caca91078ef78552c0cd9bce8f", token, ios);
        auto balance = api.token_balance(EthereumToken("0x2ba35056580b505690c03dfb1df58bc6b6cd9f89", 18));

        BOOST_CHECK(balance > 0.0);
        }
    catch (std::exception& ex)
        {
        std::cout << ex.what() << std::endl;
        BOOST_CHECK(false);
        }
}

// --run_test=test_token_balance_fail
BOOST_AUTO_TEST_CASE( test_token_balance_fail ) {
    try
        {
        boost::asio::io_service ios;

        std::string token = getenv(s_etherscan_api_token_envar_name);
        BOOST_CHECK(!token.empty());

        EthereumApi api("0xffffffffffffffffffffffffffffffffffffffff", token, ios);
        auto balance = api.token_balance(EthereumToken("0x2ba35056580b505690c03dfb1df58bc6b6cd9f89", 18));

        BOOST_CHECK(balance == 0.0);
        }
    catch (std::exception& ex)
        {
        std::cout << ex.what() << std::endl;
        BOOST_CHECK(false);
        }
}