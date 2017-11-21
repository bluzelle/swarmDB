#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "BaseClassModule"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(first)
{
    BOOST_CHECK(false);
}