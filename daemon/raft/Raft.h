#ifndef BLUZELLE_RAFT_H
#define BLUZELLE_RAFT_H

#include "PeerList.h"
#include "NodeInfo.hpp"
#include "Storage.h"
#include "CommandFactory.h"
#include "ApiCommandQueue.h"
#include "DaemonInfo.h"

#include <string>
#include <thread>
#include <queue>
#include <utility>
#include <boost/property_tree/ptree.hpp>


using std::string;
using std::queue;
using std::pair;


class Raft {
    const string s_heartbeat_message{R"({"raft":"beep"})"};
    static const uint raft_default_heartbeat_interval_milliseconds = 1050; // 50 millisec in RAFT (we need a reasonable interval, cannot expect 50ms heartbeat delivered over WAN reliably)

    boost::asio::io_service& ios_;

    PeerList peers_; // List of known peers, connected or not, some came from file some are just connected.
    //NodeInfo info_; // This node info. ---> DaemonInfo replaces this.
    Storage storage_; // Where the RAFTs log is replicated.
    ApiCommandQueue peer_queue_; // Keeps data to be sent to peers.

    CommandFactory command_factory_;

    void start_heartbeat();
    void announce_follower();
    boost::asio::deadline_timer heartbeat_timer_;
    void heartbeat();

public:
    Raft(
            boost::asio::io_service& ios
    ); // Node name, other params will be added.

    void run();

    string handle_request(const string& req);
};

#endif //BLUZELLE_RAFT_H
