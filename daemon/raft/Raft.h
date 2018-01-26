#ifndef BLUZELLE_RAFT_H
#define BLUZELLE_RAFT_H

#include <string>
#include <thread>
#include <queue>
#include <utility>
#include <mutex>

using std::queue;
using std::pair;
using std::mutex;

#include <boost/property_tree/ptree.hpp>

#include "PeerList.h"
#include "NodeInfo.hpp"
#include "Storage.h"
#include "CommandFactory.h"
#include "ApiCommandQueue.h"
#include "DaemonInfo.h"
#include "RaftState.h"


class Raft {
private:
    boost::asio::io_service& ios_;

    PeerList peers_;                    // List of known peers.

    ApiCommandQueue peer_queue_;        // Keeps data to be sent to peers.

    Storage storage_;                   // Where the RAFTs log is replicated.

    CommandFactory command_factory_;

    mutex raft_state_mutex_;

    unique_ptr<RaftState> raft_state_; // There are 3 RAFT states: Candidate, Follower and Leader.

    mutex raft_next_state_mutex_;

    unique_ptr<RaftState> raft_next_state_; // State to transition to.

public:

    explicit
    Raft(boost::asio::io_service& ios);

    void run();

    string
    handle_request(const string& req);

    void
    rearm_heartbeat_timer();

};

#endif //BLUZELLE_RAFT_H
