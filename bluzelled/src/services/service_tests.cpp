//#define BOOST_TEST_DYN_LINK

#include "services/Services.h"
#include "services/Ping.h"
#include "services/GetAllNodes.h"
#include "services/CountNodes.h"
#include "services/GetMaxNodes.h"
#include "services/GetMinNodes.h"
#include "services/SetMaxNodes.h"

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <random>

namespace pt = boost::property_tree;

pt::ptree string_to_ptree(const std::string &json_string);

std::string ptree_to_string(const pt::ptree &tree);

struct F {
    F()
    {
        std::srand((unsigned)std::time(nullptr));
    }

    ~F()
    {}
};

class TestService : public Service {
public:
    std::string operator()(const std::string &json_string) override {
        return json_string;
    }
};

// --run_test=websocket_services_tests
BOOST_FIXTURE_TEST_SUITE(websocket_services_tests, F)

// --run_test=websocket_services_tests/test_service
    BOOST_AUTO_TEST_CASE(test_service) {
        TestService sut;
        BOOST_CHECK( sut("{\"test\":1}")=="{\"test\":1}" );
    }

// --run_test=websocket_services_tests/test_service
    BOOST_AUTO_TEST_CASE(test_services) {
        std::string json_string("{\"test\":1}");
        Services sut;
        sut.add_service("test", new TestService());
        auto result = sut("test", json_string);
        BOOST_CHECK( json_string==result );
    }

// --run_test=websocket_services_tests/test_ping_service
    BOOST_AUTO_TEST_CASE(test_ping_service) {
        Ping sut;
        std::stringstream ss;
        long seq = random() % 1000000;
        ss << R"({"cmd":"ping","seq":)"
                << seq
           << "}";
        std::string request{ss.str()};

        ss.str("");
        ss << R"({"cmd":"pong","seq":)"
                << seq
           << "}";
        auto accepted_response_tree = string_to_ptree(ss.str());
        std::string accepted_response = ptree_to_string(accepted_response_tree);
        BOOST_CHECK( sut(request)==accepted_response );
    }

// --run_test=websocket_services_tests/test_get_all_nodes
//    BOOST_AUTO_TEST_CASE(test_get_all_nodes) {
//        // req {"cmd":"getAllNodes","seq":0}
//        // res {"cmd":"updateNodes","data":[{"address":"0x00","nodeState":"alive","messages":20},{"address":"0x01","nodeState":"dead","messages":20}],"seq":4}
//        Nodes nodes;
//        GetAllNodes sut(&nodes);
//
//        auto request_tree = string_to_ptree(R"({"cmd":"getAllNodes","seq":0})");
//        auto accepted_tree = string_to_ptree(R"({"cmd":"updateNodes","data":"","seq":0})");
//
//        long seq = rand() % 125000;
//        request_tree.put("seq", seq);
//        accepted_tree.put("seq", seq);
//        pt::ptree actual_tree = string_to_ptree(sut(ptree_to_string(request_tree)));
//        BOOST_CHECK(actual_tree == accepted_tree);
//
//        nodes.emplace_back(new Node());
//        nodes.emplace_back(new Node());
//        nodes.emplace_back(new Node());
//
//        actual_tree = string_to_ptree(sut(ptree_to_string(request_tree)));
//        pt::ptree child = actual_tree.get_child("data");
//
//
//        long count = 0;
//        BOOST_FOREACH(const pt::ptree::value_type &c, child)
//                        {
//                        ++count;
//                        }
//        BOOST_CHECK_EQUAL(count, nodes.size());
//
//
//        for (auto n : nodes)
//            {
//            n->kill();
//            n->join();
//            }
//
//        for (auto n : nodes)
//            {
//            delete n;
//            }
//    }

    //--run_test=websocket_services_tests/test_no_service
    BOOST_AUTO_TEST_CASE(test_no_service) {
        std::string accepted(R"({"error":"Service not found."})");
        Services sut;
        sut.add_service("test", new TestService());
        auto result = sut("test0", "{\"cmd\":\"test0\"}");
        BOOST_CHECK_MESSAGE(accepted == result, result);
    }

    //--run_test=websocket_services_tests/test_fix_json_str
    BOOST_AUTO_TEST_CASE(test_fix_json_str)
    {
        TestService sut;
        std::string json_str_in = R"({"cmd":"test","data":{"integer":"283","float":"34.433"})";
        std::string json_str_accepted;
        json_str_accepted = R"({"cmd":"test","data":{"integer":283,"float":34.433})";
        std::string json_str_actual = sut.fix_json_numbers(json_str_in);
        BOOST_CHECK_EQUAL(json_str_accepted, json_str_actual);
    }

    //--run_test=websocket_services_tests/test_count_nodes_service
    BOOST_AUTO_TEST_CASE(test_count_nodes_service)
    {
        Nodes nodes;
        CountNodes sut(&nodes);
        std::string cmd(R"({"cmd":"countNodes","seq":345})");
        std::string accepted(R"({"cmd": "nodeCount","count": 0,"seq":345})");
        std::string actual(sut(cmd));
        BOOST_CHECK(string_to_ptree(accepted) == string_to_ptree(actual));
    }

    //--run_test=websocket_services_tests/test_get_max_nodes_service
//    BOOST_AUTO_TEST_CASE(test_get_max_nodes_service)
//    {
//        GetMaxNodes sut;
//        std::string actual = sut("{\"cmd\":\"getMaxNodes\",\"seq\":222}");
//        std::cout << actual << "\n";
//        BOOST_CHECK_EQUAL
//        (
//                "{\"cmd\":\“updateMaxNodes\", \“data\": 42, \"seq\": 222}",
//                actual
//        );
//    }

BOOST_AUTO_TEST_SUITE_END()


// utiity

pt::ptree string_to_ptree(const std::string &json_string) {
    pt::ptree tree;
    std::stringstream ss;
    ss << json_string;
    pt::read_json(ss, tree);
    return tree;
}

std::string ptree_to_string(const pt::ptree &tree) {
    std::stringstream ss;
    pt::write_json(ss, tree);
    return ss.str();
}

extern unsigned long get_max_nodes()
{
    return 42;
}

