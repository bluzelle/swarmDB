#define BOOST_TEST_MODULE "BaseClassModule"

#include <boost/range.hpp>
#include <boost/test/unit_test.hpp>

#include "CommandLineOptions.h"


//  --run_test=test_options_all
BOOST_AUTO_TEST_CASE(test_options_all) {
    char *argv[] =
            {
                    (char *) "~/bluzelle/test",
                    (char *) "--address",
                    (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                    (char *) "--port",
                    (char *) "58000",
                    (char *) "--config",
                    (char *) "~/.bluzellerc",
            };

    CommandLineOptions op;
    op.parse(7, argv);

    BOOST_CHECK_EQUAL(op.get_address(), "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89");
    BOOST_CHECK_EQUAL(op.get_port(), 58000);
    BOOST_CHECK(op.get_config() == "~/.bluzellerc");
}

//  --run_test=test_options_missing_address
BOOST_AUTO_TEST_CASE(test_options_missing_address) {
    char *argv[] =
            {
                    (char *) "~/bluzelle/test",
                    (char *) "--port",
                    (char *) "58000",
                    (char *) "--config",
                    (char *) "~/.bluzellerc",
            };

    try
        {
        CommandLineOptions op;
        op.parse(5, argv);

        BOOST_CHECK_EQUAL(op.get_address(), "");
        }
    catch (...)
        {
        BOOST_CHECK(false);
        }
}

//  --run_test=test_options_missing_port
BOOST_AUTO_TEST_CASE(test_options_missing_port) {
    char *argv[] =
            {
                    (char *) "~/bluzelle/test",
                    (char *) "--address",
                    (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                    (char *) "--config",
                    (char *) "~/.bluzellerc",
            };

    try
        {
        CommandLineOptions op;
        op.parse(5, argv);

        BOOST_CHECK_EQUAL(op.get_port(), 0);
        }
    catch (...)
        {
        BOOST_CHECK(false);
        }
}

//  --run_test=test_options_missing_config
BOOST_AUTO_TEST_CASE(test_options_missing_config) {
    char *argv[] =
            {
                    (char *) "~/bluzelle/test",
                    (char *) "--address",
                    (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                    (char *) "--port",
                    (char *) "58000",
            };

    try
        {
        CommandLineOptions op;
        op.parse(5, argv);

        BOOST_CHECK_EQUAL(op.get_config(), "");
        }
    catch (...)
        {
        BOOST_CHECK(false);
        }
}

//  --run_test=test_options_address_validation
BOOST_AUTO_TEST_CASE(test_options_address_validation) {
    CommandLineOptions op;

    bool valid = op.is_valid_ethereum_address("0x2ba35056580b505690c03dfb1df58bc6b6cd9f89");
    BOOST_CHECK_EQUAL(valid, true);

    valid = op.is_valid_ethereum_address("0X2ba35056580b505690c03dfb1df58bc6b6cd9f89");
    BOOST_CHECK_EQUAL(valid, true);
}

//  --run_test=test_options_address_validation_fail
BOOST_AUTO_TEST_CASE(test_options_address_validation_fail) {
    CommandLineOptions op;

    bool valid = op.is_valid_ethereum_address("hello, world"); // Invalid characters.
    BOOST_CHECK_EQUAL(valid, false);

    valid = op.is_valid_ethereum_address("0X2ba35056580b505690c03dfb1d"); // Invalid size.
    BOOST_CHECK_EQUAL(valid, false);
}

