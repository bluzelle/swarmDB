#define BOOST_TEST_MODULE "BaseClassModule"
#include <boost/test/unit_test.hpp>

struct F
{
    F() = default;

    ~F() = default;
};


BOOST_FIXTURE_TEST_SUITE(base_suite, F)
    BOOST_AUTO_TEST_CASE(first)
    {
        BOOST_CHECK(true);
    }
BOOST_AUTO_TEST_SUITE_END()