#include "DaemonInfo.h"

#include <boost/test/unit_test.hpp>
#include <vector>

using namespace std;

struct F
{
    F() = default;
    ~F() = default;
};

BOOST_FIXTURE_TEST_SUITE(daemoninfo_suite, F)

    BOOST_AUTO_TEST_CASE(test_values)
    {
        DaemonInfo& sut = DaemonInfo::get_instance();
        string accepted_ip = "128.122.0.22";
        string accepted_name = "jeep";
        uint16_t accepted_port = 35473;
        sut.host_ip() = accepted_ip;
        sut.host_port() = accepted_port;
        sut.host_name() = accepted_name;

        BOOST_CHECK_EQUAL(accepted_ip, sut.host_ip());
        BOOST_CHECK_EQUAL(accepted_port, sut.host_port());
        BOOST_CHECK_EQUAL(accepted_name, sut.host_name());
    }

BOOST_AUTO_TEST_SUITE_END()