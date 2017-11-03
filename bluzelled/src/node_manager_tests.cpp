#define BOOST_TEST_DYN_LINK

#include "NodeManager.h"
#include <web_sockets/Listener.h>

#include <boost/test/unit_test.hpp>

bool all_alive( NodeManager& nm);

struct F
{
    F() = default;

    ~F() = default;
};


BOOST_FIXTURE_TEST_SUITE(node_manager_tests, F)

    // --run_test=node_manager_tests/test_node_manager
    BOOST_AUTO_TEST_CASE(constructor)
    {
        NodeManager sut;
        BOOST_CHECK_EQUAL(sut.min_nodes(), 5);
        BOOST_CHECK_EQUAL(sut.max_nodes(), 25);
        BOOST_CHECK_GE(sut.nodes().capacity(), 25);
    }

    BOOST_AUTO_TEST_CASE(param_constructor)
    {
        const size_t actual_min = 3;
        const size_t actual_max = 33;
        NodeManager sut(actual_min, actual_max);
        BOOST_CHECK_EQUAL(sut.min_nodes(), actual_min);
        BOOST_CHECK_EQUAL(sut.max_nodes(), actual_max);
        BOOST_CHECK_GE(sut.nodes().capacity(), actual_max);
    }

    BOOST_AUTO_TEST_CASE(setters_getters)
    {
        // yes yes, I know, testing [s/g]etters is pedantic, but
        // in this case there are some error conditions that need
        // testing.
        const size_t actual_min = 3;
        const size_t actual_max = 33;
        NodeManager sut;

        sut.set_min_nodes(actual_min);
        BOOST_CHECK_EQUAL(sut.min_nodes(), actual_min);

        sut.set_max_nodes(actual_max);
        BOOST_CHECK_EQUAL(sut.max_nodes(), actual_max);

        BOOST_CHECK_THROW(
                sut.set_min_nodes(sut.max_nodes()), std::runtime_error);
        BOOST_CHECK_EQUAL(sut.min_nodes(), actual_min);

        BOOST_CHECK_THROW(
                sut.set_max_nodes(sut.min_nodes()), std::runtime_error
        );

        BOOST_CHECK_EQUAL(sut.max_nodes(), actual_max);
    }

    BOOST_AUTO_TEST_CASE(create_and_add_nodes)
    {
        std::shared_ptr<Listener> l;
        NodeManager sut;
        sut.create_and_add_nodes( l, 5);
        BOOST_CHECK_EQUAL(5, sut.nodes_count());
        sut.create_and_add_nodes( l, 5);
        BOOST_CHECK_EQUAL(10, sut.nodes_count());

    }

    BOOST_AUTO_TEST_CASE(kill)
    {
        std::shared_ptr<Listener> l;
        NodeManager sut;
        sut.create_and_add_nodes( l, 25);
        while(!all_alive(sut))
            {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            }

        sut.kill_all_nodes();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));

        bool all_dead = true;
        for(auto &n : sut.nodes())
            {
            if(Task::dead != n->state())
                {
                all_dead = false;
                break;
                }
            }
        BOOST_CHECK(all_dead);
    }

    BOOST_AUTO_TEST_CASE(all_nodes_alive)
    {
        std::shared_ptr<Listener> l;
        NodeManager sut;
        sut.create_and_add_nodes( l, 25);

        while(!all_alive(sut))
            {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            }
        BOOST_CHECK(sut.all_nodes_alive());
        sut.kill_all_nodes();
        BOOST_CHECK(!sut.all_nodes_alive());
    }

BOOST_AUTO_TEST_SUITE_END()

///////////////////////////////////////////////////////////////////////////////
bool all_alive( NodeManager& nm)
    {
    for(auto& n : nm.nodes())
        {
        if(n->state() != Task::State::alive)
            {
            return false;
            }
        }
    return true;
    }
