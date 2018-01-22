// See KEP-87
#include "node/NodeInfo.hpp"

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

struct F
{
    F() = default;

    ~F() = default;
};

using boost::uuids::uuid;

// --run_test=node_info
BOOST_FIXTURE_TEST_SUITE(node_info_tests, F)

    BOOST_AUTO_TEST_CASE( test_setter_and_getter )
    {
        NodeInfo sut;
        const std::string accepted_host = "198.162.0.22";
        const uint16_t accepted_port = 34395;
        sut.host() = accepted_host;
        sut.port() = accepted_port;

        BOOST_CHECK_EQUAL( accepted_host, sut.host());
        BOOST_CHECK_EQUAL( accepted_port, sut.port());
    }

BOOST_AUTO_TEST_SUITE_END()
