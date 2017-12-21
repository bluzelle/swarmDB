#ifndef BLUZELLE_PEER_H
#define BLUZELLE_PEER_H

#include "NodeInfo.hpp"
#include "PeerSession.h"

#include <memory>

using std::shared_ptr;

class Peer
{
    boost::asio::io_service& ios_;
    NodeInfo info_; // This node info.
    shared_ptr<PeerSession> session_; // Corresponding session.
public:

    Peer(
            boost::asio::io_service& ios,
            NodeInfo &node_info
    )  : ios_(ios),
         info_(node_info)
    {
    }

    string send_request(const string& req);
};

#endif //BLUZELLE_PEER_H
