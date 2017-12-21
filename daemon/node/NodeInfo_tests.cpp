// See KEP-87
#include "node/NodeInfo.hpp"

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
BOOST_FIXTURE_TEST_SUITE(node_info, F)

    BOOST_AUTO_TEST_CASE( test_setter_and_getter )
    {
        NodeInfo sut;
        const string key = "key42";
        const int accepted_value = 42;
        sut.set_value( key, accepted_value);
        auto value = sut.get_value<int>(key);
        BOOST_CHECK(accepted_value == value);
    }

    BOOST_AUTO_TEST_CASE(test_type_handling)
    {
        NodeInfo sut;
        boost::uuids::basic_random_generator<boost::mt19937> gen;

        const ushort actual_host = 127;
        const u_int32_t actual_port = 54000;

        const boost::uuids::uuid actual_address{gen()};
        const std::string actual_address_str = boost::lexical_cast<std::string>(actual_address);

        const std::string actual_node_name{"Sagan"};

        const boost::uuids::uuid actual_node_id = gen();
        const std::string actual_node_id_str = boost::lexical_cast<std::string>(actual_node_id);

        sut.set_value("host", actual_host);
        sut.set_value("port", actual_port);
        sut.set_value("address",  actual_address_str);
        sut.set_value("name", actual_node_name);
        sut.set_value("node_id", boost::lexical_cast<std::string>(actual_node_id));

        BOOST_CHECK(sut.get_value<ushort>("host") == actual_host);
        BOOST_CHECK(sut.get_value<u_int32_t>("port") == actual_port);
        BOOST_CHECK(sut.get_value<std::string>("address") == actual_address_str);
        BOOST_CHECK(sut.get_value<std::string>("name") == actual_node_name);
        BOOST_CHECK(sut.get_value<std::string>("node_id") == actual_node_id_str);
    }

    BOOST_AUTO_TEST_CASE(test_missing_value)
    {
        NodeInfo sut;
        const string key = "MissingKey";
        unsigned long value = 22;
        BOOST_CHECK_THROW(value = sut.get_value<unsigned long>(key), std::exception);
        BOOST_CHECK( value == 0);
    }

BOOST_AUTO_TEST_SUITE_END()
