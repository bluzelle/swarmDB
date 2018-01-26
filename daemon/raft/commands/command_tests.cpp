#include "ApiCreateCommand.h"
#include "ApiReadCommand.h"

#include <boost/test/unit_test.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using namespace std;
namespace bpt = boost::property_tree;
namespace bu = boost::uuids;
namespace br = boost::random;

//boost::mt19937 ran;
//bu::basic_random_generator<boost::mt19937> gen(&ran);
//br::uniform_int_distribution<> random_unsigned_char(0,UCHAR_MAX);

struct F
{
    F() = default;

    ~F() = default;
};


BOOST_FIXTURE_TEST_SUITE(commands_suite, F)

    BOOST_AUTO_TEST_CASE(test_api_create)
    {
        ApiCommandQueue queue;
        Storage storage;
        string accepted_value{"value_222"};
        bpt::ptree pt = pt_from_json_string
            (
                R"({"bzn-api":"create", "transaction-id":"123", "data":{"key":"key_222", "value":"value_222"}})"
            );

        ApiCreateCommand sut
            (
            queue,
            storage,
            pt
        );

        // "{\n    \"result\": \"success\"\n}\n"
        bpt::ptree response = sut();

        string result = response.get<string>("result");
        BOOST_CHECK_EQUAL("success", result);

        Record r = storage.read("key_222");
        BOOST_CHECK_EQUAL(accepted_value, r.value());
    }

    BOOST_AUTO_TEST_CASE(test_api_read)
    {
        ApiCommandQueue queue;
        Storage storage;

        bpt::ptree pt = pt_from_json_string
            (
                R"({"bzn-api":"read", "transaction-id":"c41b3c8b-634e-435a-b582-96b83bbd4032", "data":{"key":"key_222"}})"
            );

        string accepted_key{"test_key"};
        string accepted_value{"test_api_read"};

        storage.remove( accepted_key);
        storage.create(
            accepted_key,
            accepted_value,
            "c41b3c8b-634e-435a-b582-96b83bbd4032"
        );

        try
            {
            ApiReadCommand sut(queue, storage, pt);
            bpt::ptree response = sut();

            stringstream ss;
            bpt::write_json(ss, response);
            cout << "[" << ss.str() << "]\n";


            BOOST_CHECK_EQUAL("read", response.get<string>("bzn-api"));
            BOOST_CHECK_EQUAL("c41b3c8b-634e-435a-b582-96b83bbd4032", response.get<string>("transaction-id"));
            BOOST_CHECK_EQUAL("key_222", response.get<string>("data.key"));

            }
        catch(const exception &e)
            {
            cerr << "Exception in test_api_read:" << e.what() << "\n";
            }










    }





BOOST_AUTO_TEST_SUITE_END()