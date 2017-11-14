#define BOOST_TEST_DYN_LINK

#include "Node.h"
#include "NodeUtilities.h"

#include <vector>
#include <boost/test/unit_test.hpp>
#include <numeric>

std::shared_ptr<Listener> l;

Nodes* create_test_nodes(int n);
unsigned count_alive(const Nodes nodes);
unsigned count_dead(const Nodes nodes)
{
    return nodes.size() - count_alive(nodes);
}


static Nodes s_nodes;

Nodes *get_all_nodes()
{
    return &s_nodes;
}

unsigned long s_max_nodes = 25;

void set_max_nodes(unsigned long max)
{
    s_max_nodes = max;
}


// --run_test=test_node_death
BOOST_AUTO_TEST_CASE( test_node_death )
{
    auto test = [](time_t actual, double prob)
        {
        time_t elapsed = 0;
        time_t start = 0;
        Node sut(l, actual, prob);
        while(sut.state() !=  Task::State::alive){}
        start = time(nullptr);
        BOOST_CHECK_EQUAL(sut.state(), Task::State::alive);
        while(sut.state() ==  Task::State::alive){}
        while(sut.state() ==  Task::State::dying){}

        BOOST_CHECK_EQUAL(sut.state(), Task::State::dead);
        elapsed = time(nullptr) - start;
        BOOST_CHECK( abs(elapsed - actual) < 1 );
        sut.join();
        };
    test( 1, 1.0);
    test( 2, 1.0);
    test( 3, 1.0);
    // TODO: How to test the probability?
}


//  --run_test=test_generate_ip_address
BOOST_AUTO_TEST_CASE( test_generate_ip_address ) {
    boost::system::error_code ec;

    std::vector<std::string>ip;

    for (int i = 0; i < 100; ++ i)
        {
        auto s = generate_ip_address();
        ip.push_back(s);
        }

    // Check uniqueness.
    auto it = std::unique(ip.begin(), ip.end());
    BOOST_CHECK(it == ip.end());

    for (auto& s : ip)
        {
        boost::asio::ip::address::from_string(s, ec); // Validates IPv4 and IPv6 addresses.
        BOOST_CHECK(ec == boost::system::errc::errc_t::success);
        }
}

//  --run_test=test_reaper
//BOOST_AUTO_TEST_CASE( test_reaper )
//{
//    const int TEST_NODE_COUNT = 3;
//    Nodes* nodes = create_test_nodes(TEST_NODE_COUNT);
//
//    BOOST_CHECK( TEST_NODE_COUNT == nodes->size() );
//    wait_for_all_nodes_to_start(*nodes);
//    reaper(nodes);
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//    BOOST_CHECK( 3 == nodes->size() );
//
//    nodes->operator[](1)->kill();
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//    BOOST_CHECK(2==count_alive(*nodes));
//
//    reaper(nodes);
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//    BOOST_CHECK( 2 == nodes->size() );
//    BOOST_CHECK( 2 == count_alive(*nodes) );
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
//
//    for(auto n : *nodes)
//        {
//        n->kill();
//        }
//
//    BOOST_CHECK( 2 == nodes->size() );
//    BOOST_CHECK( 0 == count_alive(*nodes) );
//
//    reaper(nodes);
//    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
//
//    BOOST_CHECK_MESSAGE( 0 == nodes->size(), " nodes->size() is [" << nodes->size() << "].");
//
//    delete nodes;
//}

//BOOST_AUTO_TEST_CASE( test_reaper_large_population )
//{
//    srand(time(nullptr));
//    const int TEST_NODE_COUNT = 30;
//    Nodes* nodes = create_test_nodes(TEST_NODE_COUNT);
//    wait_for_all_nodes_to_start(*nodes);
//
//    BOOST_CHECK(TEST_NODE_COUNT == count_alive(*nodes));
//
//    // kill off a third
//    const int killed = TEST_NODE_COUNT/3;
//    std::set<int> indexes;
//    while(indexes.size()<killed)
//        {
//        indexes.emplace(rand() % TEST_NODE_COUNT);
//        }
//
//    for(auto index : indexes)
//        {
//        (*nodes)[index]->kill();
//        (*nodes)[index]->join();
//        }
//
//    BOOST_CHECK(TEST_NODE_COUNT - killed == count_alive(*nodes));
//    BOOST_CHECK(TEST_NODE_COUNT == (*nodes).size());
//
//    reaper(nodes);
//
//    BOOST_CHECK(TEST_NODE_COUNT - killed == count_alive(*nodes));
//    BOOST_CHECK(TEST_NODE_COUNT - killed == (*nodes).size());
//    for(auto n : *nodes)
//        {
//        n->kill();
//        n->join();
//        delete n;
//        }
//}

Nodes* create_test_nodes(int n)
{
    Nodes* nodes = new Nodes();
    for(int i = 0; i < n ; ++i)
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
