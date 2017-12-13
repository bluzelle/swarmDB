#ifndef BLUZELLE_PEER_H
#define BLUZELLE_PEER_H

#include <memory>

using std::shared_ptr;

#include "NodeInfo.hpp"
#include "PeerSession.h"

class Peer {
public:
    NodeInfo info_; // This node info.
    shared_ptr<PeerSession> session_; // Corresponding session.
    boost::asio::io_service& ios_;

    Peer(boost::asio::io_service& ios, NodeInfo i);

    string send_request(const string& req);
    //string handle_response(const string& req);
};

#endif //BLUZELLE_PEER_H
