#define BOOST_TEST_MODULE "BaseClassModule"

#include <boost/test/unit_test.hpp>

//  --run_test=test_raft_leader_election
BOOST_AUTO_TEST_CASE(test_raft_leader_election) {
    BOOST_CHECK(false);
}

/*
 * Read PeerList
 * Spawn nodes, wait for leader election to complete
 * Connect to any node and store data
 * Conncet to another node and update data
 * Connect to 3rd node and read data
 * Connect to 4th node and delete dat
 * Connect to 5th node and read again (Error)
 */


//  --run_test=test_raft_crud
BOOST_AUTO_TEST_CASE(test_raft_crud) {
    BOOST_CHECK(false);
}