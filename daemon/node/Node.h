#ifndef BLUZELLE_NODE_H
#define BLUZELLE_NODE_H

#include "PeerServer.h"
#include "NodeInfo.hpp"
#include "MessageInfo.hpp"
#include "Raft.h"

#include <vector>
#include <string>

using namespace std;

class Node {
private:
    Raft        raft_; // RAFT protocol state machine.
    PeerServer  server_; // Serves inbound connections.

public:
    Node(boost::asio::io_service& ios);

    void
    run();

    vector<NodeInfo>
    get_peers() const; // returns list of all known peers connected or not.

    vector<MessageInfo>
    get_messages() const; // returns list of messages sent by this node, since timestamp.
};

#endif //BLUZELLE_NODE_H
