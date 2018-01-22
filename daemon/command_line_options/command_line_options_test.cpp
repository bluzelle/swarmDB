#include "CommandLineOptions.h"
#include "ParseUtils.h"

#include <iostream>
#include <boost/range.hpp>
#include <boost/test/unit_test.hpp>

struct F
{
    F() = default;

    ~F() = default;
};

// --run_test=websockets
BOOST_FIXTURE_TEST_SUITE(command_line_options, F)

    BOOST_AUTO_TEST_CASE(test_parse_utils_sim_delay_valid)
    {
        try
            {
            CommandLineOptions clo;
            const char *argv[] =
                {
                    (char *) "~/bluzelle/test",
                    (char *) "--address",
                    (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                    (char *) "--port",
                    (char *) "58000"
                };

            clo.parse(5,argv);
            BOOST_CHECK(simulated_delay_values_within_bounds(clo));

            const char *argv_good[] =
                {
                    (char *) "~/bluzelle/test",
                    (char *) "--address",
                    (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                    (char *) "--port",
                    (char *) "58000",
                    (char *) "--simulated_delay_lower",
                    (char *) "10",
                    (char *) "--simulated_delay_upper",
                    (char *) "2000"
                };
            clo.parse(9,argv_good);
            BOOST_CHECK(simulated_delay_values_within_bounds(clo));

            const char *argv_low_too_low[] =
                {
                    (char *) "~/bluzelle/test",
                    (char *) "--address",
                    (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                    (char *) "--port",
                    (char *) "58000",
                    (char *) "--simulated_delay_lower",
                    (char *) "5",
                    (char *) "--simulated_delay_upper",
                    (char *) "2000"
                };
            clo.parse(9,argv_low_too_low);
            BOOST_CHECK(!simulated_delay_values_within_bounds(clo));

            const char *argv_low_higher_than_high[] =
                {
                    (char *) "~/bluzelle/test",
                    (char *) "--address",
                    (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                    (char *) "--port",
                    (char *) "58000",
                    (char *) "--simulated_delay_lower",
                    (char *) "1500",
                    (char *) "--simulated_delay_upper",
                    (char *) "1000"
                };
            clo.parse( 9,argv_low_higher_than_high);
            BOOST_CHECK(!simulated_delay_values_within_bounds(clo));

            const char *argv_upper_too_high[] =
                {
                    (char *) "~/bluzelle/test",
                    (char *) "--address",
                    (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                    (char *) "--port",
                    (char *) "58000",
                    (char *) "--simulated_delay_lower",
                    (char *) "1500",
                    (char *) "--simulated_delay_upper",
                    (char *) "2001"
                };
            clo.parse( 9, argv_upper_too_high);
            BOOST_CHECK(!simulated_delay_values_within_bounds(clo));
            }
        catch(...)
            {
            BOOST_CHECK(false);
            }


    }

    BOOST_AUTO_TEST_CASE(test_parse_utils_delay_set)
    {
        const char *argv_set[] =
            {
                (char *) "~/bluzelle/test",
                (char *) "--address",
                (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                (char *) "--port",
                (char *) "58000",
                (char *) "--simulated_delay_lower",
                (char *) "10",
                (char *) "--simulated_delay_upper",
                (char *) "2000"
            };


        const char *argv_notset[] =
            {
                (char *) "~/bluzelle/test",
                (char *) "--address",
                (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                (char *) "--port",
                (char *) "58000"
            };

        try
            {
            CommandLineOptions clo;
            clo.parse( 9, argv_set);
            BOOST_CHECK(simulated_delay_values_set(clo));

            clo.parse( 5, argv_notset);
            BOOST_CHECK_EQUAL( false, simulated_delay_values_set(clo));
            }
        catch (...)
            {
            BOOST_CHECK(false);
            }

    }

    BOOST_AUTO_TEST_CASE(test_sim_delay_lower_upper)
    {
        const char *argv[] =
            {
                (char *) "~/bluzelle/test",
                (char *) "--address",
                (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                (char *) "--port",
                (char *) "58000",
                (char *) "--simulated_delay_lower",
                (char *) "10",
                (char *) "--simulated_delay_upper",
                (char *) "2000"
            };

        try
            {
            CommandLineOptions sut;
            bool r = sut.parse(9, argv);
            BOOST_CHECK(r);
            BOOST_CHECK_EQUAL(10, sut.get_simulated_delay_lower());
            BOOST_CHECK_EQUAL(2000, sut.get_simulated_delay_upper());
            }
        catch(const std::exception& e)
            {
            std::cerr << "Exception:[" << e.what() << "]\n";
            BOOST_CHECK(false);
            }
    }

    BOOST_AUTO_TEST_CASE(test_no_sim_delay)
    {
        const char *argv[] =
            {
                (char *) "~/bluzelle/test",
                (char *) "--address",
                (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                (char *) "--port",
                (char *) "58000"
            };
        try
            {
            CommandLineOptions sut;
            bool r = sut.parse(5,argv);
            BOOST_CHECK(r);
            BOOST_CHECK_EQUAL(0, sut.get_simulated_delay_lower());
            BOOST_CHECK_EQUAL(0, sut.get_simulated_delay_upper());
            }
        catch(const std::exception& e)
            {
            std::cerr << "Exception:[" << e.what() << "]\n";
            BOOST_CHECK(false);
            }
    }

    BOOST_AUTO_TEST_CASE(test_2nd_sim_delay)
    {
        const char *argv[] =
            {
                (char *) "~/bluzelle/test",
                (char *) "--address",
                (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                (char *) "--port",
                (char *) "58000",
                (char *) "-l",
                (char *) "10",
                (char *) "-u",
                (char *) "2000"
            };
        try
            {
            CommandLineOptions sut;
            bool r = sut.parse(9,argv);
            BOOST_CHECK(r);
            BOOST_CHECK_EQUAL(10, sut.get_simulated_delay_lower());
            BOOST_CHECK_EQUAL(2000, sut.get_simulated_delay_upper());
            }
        catch(const std::exception& e)
            {
            std::cerr << "Exception:[" << e.what() << "]\n";
            BOOST_CHECK(false);
            }
    }


    BOOST_AUTO_TEST_CASE(test_sim_delay_setters)
    {
        const char *argv[] =
            {
                (char *) "~/bluzelle/test",
                (char *) "--address",
                (char *) "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89",
                (char *) "--port",
                (char *) "58000",
                (char *) "-l",
                (char *) "10",
                (char *) "-u",
                (char *) "2000"
            };


        try
            {
            CommandLineOptions sut;
            bool r = sut.parse(9,argv);
            BOOST_CHECK(r);
            BOOST_CHECK_EQUAL(sut.get_simulated_delay_lower(),10);
            BOOST_CHECK_EQUAL(sut.get_simulated_delay_upper(),2000);
            }
        catch(const std::exception& e)
            {
            std::cerr << "Exception:[" << e.what() << "]\n";
            BOOST_CHECK(false);
            }
    }


    //  --run_test=websockets/test_options_all
    BOOST_AUTO_TEST_CASE(test_options_all)
    {
        const char *argv[] =
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

//  --run_test=websockets/test_options_missing_address
    BOOST_AUTO_TEST_CASE(test_options_missing_address)
    {
        const char *argv[] =
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

//  --run_test=websockets/test_options_missing_port
    BOOST_AUTO_TEST_CASE(test_options_missing_port)
    {
        const char *argv[] =
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
    BOOST_AUTO_TEST_CASE(test_options_missing_config)
    {
        const char *argv[] =
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
    BOOST_AUTO_TEST_CASE(test_options_address_validation)
    {
        CommandLineOptions op;

        bool valid = op.is_valid_ethereum_address("0x2ba35056580b505690c03dfb1df58bc6b6cd9f89");
        BOOST_CHECK_EQUAL(valid, true);

        valid = op.is_valid_ethereum_address("0X2ba35056580b505690c03dfb1df58bc6b6cd9f89");
        BOOST_CHECK_EQUAL(valid, true);
    }

//  --run_test=test_options_address_validation_fail
    BOOST_AUTO_TEST_CASE(test_options_address_validation_fail)
    {
        CommandLineOptions op;

        bool valid = op.is_valid_ethereum_address("hello, world"); // Invalid characters.
        BOOST_CHECK_EQUAL(valid, false);

        valid = op.is_valid_ethereum_address("0X2ba35056580b505690c03dfb1d"); // Invalid size.
        BOOST_CHECK_EQUAL(valid, false);
    }

BOOST_AUTO_TEST_SUITE_END()