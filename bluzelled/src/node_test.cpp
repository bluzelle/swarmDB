#define BOOST_TEST_DYN_LINK

#include "Node.h"
#include "NodeUtilities.h"

#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <sstream>

std::shared_ptr<Listener> l;

Nodes *create_test_nodes(int n);

struct F
{
    F()
    {
    }

    ~F()
    {}
};


BOOST_FIXTURE_TEST_SUITE(node_tests, F)


    // run_test==node_tests/constructor
    BOOST_AUTO_TEST_CASE(constructor)
    {
        using namespace boost::chrono;
        //    Node(std::shared_ptr<Listener> l, uint32_t lifespan = 20, double death_probablity = 0.05);
        Node sut(l);

        // name should be ipv4
        boost::regex expression("([0-9]{1,3}\\.{0,1})*");
        boost::cmatch what;
        BOOST_CHECK_EQUAL(true, regex_match(sut.name().c_str(), what, expression));
        std::stringstream ss;
        ss << what;
        BOOST_CHECK_EQUAL( ss.str(), sut.name().c_str());
    }

    BOOST_AUTO_TEST_CASE(state)
    {
        //    Task::State state();
        Node sut(l);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
        BOOST_CHECK(Task::State::initializing==sut.state() || Task::State::alive ==sut.state());
        sut.kill();
        BOOST_CHECK(Task::State::dying==sut.state() || Task::State::dead ==sut.state());
        boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
        BOOST_CHECK(Task::State::dead ==sut.state());
        sut.join();
    }

//    void kill();
    BOOST_AUTO_TEST_CASE(kill)
    {
        Node sut(l);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        BOOST_CHECK(Task::State::alive ==sut.state());
        sut.kill();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        BOOST_CHECK(Task::State::dead ==sut.state() || Task::State::dying ==sut.state());
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        BOOST_CHECK(Task::State::dead ==sut.state());
        sut.join();
    }

//    system_clock::time_point last_change();


    // --run_test=test_node_death
    BOOST_AUTO_TEST_CASE(natural_death)
    {
        Node sut(l,1,0.9);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        BOOST_CHECK(Task::State::alive ==sut.state());
        boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
        BOOST_CHECK(Task::State::dead ==sut.state());
        sut.join();
    }


    //  --run_test=test_generate_ip_address
    BOOST_AUTO_TEST_CASE(test_generate_ip_address)
    {
        boost::system::error_code ec;
        std::vector<std::string> ip;

        for (int i = 0; i < 100; ++i)
            {
            auto s = generate_ip_address();
            ip.push_back(s);
            }

        // Check uniqueness.
        auto it = std::unique(ip.begin(), ip.end());
        BOOST_CHECK(it == ip.end());

        for (auto &s : ip)
            {
            boost::asio::ip::address::from_string(s, ec); // Validates IPv4 and IPv6 addresses.
            BOOST_CHECK(ec == boost::system::errc::errc_t::success);
            }
    }

    // TODO: move reaper test to node manager tests
    //  --run_test=test_reaper
//    BOOST_AUTO_TEST_CASE(test_reaper)
//    {
//        const int TEST_NODE_COUNT = 3;
//        Nodes *nodes = create_test_nodes(TEST_NODE_COUNT);
//
//        BOOST_CHECK(TEST_NODE_COUNT == nodes->size());
//        wait_for_all_nodes_to_start(*nodes);
//        reaper(nodes);
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//        BOOST_CHECK(3 == nodes->size());
//
//        nodes->operator[](1)->kill();
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//        BOOST_CHECK(2 == count_alive(*nodes));
//
//        reaper(nodes);
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//        BOOST_CHECK(2 == nodes->size());
//        BOOST_CHECK(2 == count_alive(*nodes));
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//        for (auto n : *nodes)
//            {
//            n->kill();
//            }
//
//        BOOST_CHECK(2 == nodes->size());
//        BOOST_CHECK(0 == count_alive(*nodes));
//
//        reaper(nodes);
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
//
//        BOOST_CHECK_MESSAGE(0 == nodes->size(), " nodes->size() is [" << nodes->size() << "].");
//
//        delete nodes;
//    }

//  --run_test=test_reaper_large_population
//    BOOST_AUTO_TEST_CASE(test_reaper_large_population)
//    {
//        srand(time(nullptr));
//        const int TEST_NODE_COUNT = 30;
//        Nodes *nodes = create_test_nodes(TEST_NODE_COUNT);
//        wait_for_all_nodes_to_start(*nodes);
//
//        BOOST_CHECK(TEST_NODE_COUNT == count_alive(*nodes));
//
//        // kill off a third
//        const int killed = TEST_NODE_COUNT / 3;
//        std::set<int> indexes;
//        while (indexes.size() < killed)
//            {
//            indexes.emplace(rand() % TEST_NODE_COUNT);
//            }
//
//        for (auto index : indexes)
//            {
//            (*nodes)[index]->kill();
//            (*nodes)[index]->join();
//            }
//
//        BOOST_CHECK(TEST_NODE_COUNT - killed == count_alive(*nodes));
//        BOOST_CHECK(TEST_NODE_COUNT == (*nodes).size());
//
//        reaper(nodes);
//
//        BOOST_CHECK(TEST_NODE_COUNT - killed == count_alive(*nodes));
//        BOOST_CHECK(TEST_NODE_COUNT - killed == (*nodes).size());
//        for (auto n : *nodes)
//            {
//            n->kill();
//            n->join();
//            delete n;
//            }
//    }

//  --run_test=test_all_nodes_alive
//    BOOST_AUTO_TEST_CASE(test_all_nodes_alive)
//    {
//        Nodes nodes;
//        Node node0(l);
//        Node node1(l);
//        Node node2(l);
//
//        BOOST_CHECK(!all_nodes_alive(nodes));
//
//        nodes.emplace_back(&node0);
//        nodes.emplace_back(&node1);
//        nodes.emplace_back(&node2);
//
//        // NOTE: the assumption here is that 100ms is long enough for 3 nodes to
//        // get initialized and running in the alive state.
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//        BOOST_CHECK(all_nodes_alive(nodes));
//        node0.kill();
//        BOOST_CHECK(!all_nodes_alive(nodes));
//        node0.join();
//        BOOST_CHECK(!all_nodes_alive(nodes));
//
//        node1.kill();
//        node1.join();
//
//        node2.kill();
//        node2.join();
//    }

//  --run_test=test_kill_and_join_all_nodes
//    BOOST_AUTO_TEST_CASE(test_kill_and_join_all_nodes)
//    {
//        const uint k_number_of_nodes = 5;
//        Nodes *nodes = create_test_nodes(k_number_of_nodes);
//        BOOST_CHECK_EQUAL(k_number_of_nodes, nodes->size());
//
//        wait_for_all_nodes_to_start(*nodes);
//        BOOST_CHECK(all_nodes_alive(*nodes));
//        kill_and_join_all_nodes(*nodes);
//        BOOST_CHECK(!all_nodes_alive(*nodes));
//
//        nodes->clear();
//        delete nodes;
//    }

//  --run_test=test_create_node
//    BOOST_AUTO_TEST_CASE(test_create_node)
//    {
//        Node *n = create_node(l);
//        BOOST_CHECK(nullptr != n);
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
//        BOOST_CHECK_EQUAL(n->state(), Task::alive);
//        n->kill();
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
//        BOOST_CHECK_EQUAL(n->state(), Task::dead);
//        BOOST_CHECK_EQUAL(n->is_joinable(), true);
//        n->join();
//    }

//    BOOST_AUTO_TEST_CASE(test_create_nodes)
//    {
//        unsigned k_node_count = 22;
//        Nodes nodes = create_nodes(k_node_count, l);
//        while (!all_nodes_alive(nodes))
//            {};
//        BOOST_CHECK_EQUAL(true, all_nodes_alive(nodes));
//        BOOST_CHECK_EQUAL(k_node_count, nodes.size());
//        kill_and_join_all_nodes(nodes);
//    }


//    BOOST_AUTO_TEST_CASE(test_add_nodes)
//    {
////        unsigned int node_count = 0;
////        NodeManager nodeManager();
////
////        Nodes nodes;
////        BOOST_CHECK_EQUAL(node_count, nodes.size());
//
//
//    }


BOOST_AUTO_TEST_SUITE_END()


////////////////////////////////
Nodes *create_test_nodes(int n)
{
    Nodes *nodes = new Nodes();
    for (int i = 0; i < n; ++i)
        {
        nodes->emplace_back(new Node(l));
        }
    return nodes;
}

unsigned count_alive(const Nodes nodes)
{
    return std::accumulate(
            nodes.begin(),
            nodes.end(),
            0,
            [](auto p, auto node)
                {
                return p + ((Task::alive == node->state()) ? 1 : 0);
                }
    );
}
