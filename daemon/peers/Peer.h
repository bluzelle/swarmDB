#ifndef BLUZELLE_PEER_H
#define BLUZELLE_PEER_H

#include "NodeInfo.hpp"
#include "PeerSession.h"

#include <memory>

using namespace std;

class Peer
{
private:
    boost::asio::io_service& ios_;
    NodeInfo info_; // This node info.
public:

    Peer(
            boost::asio::io_service& ios,
            NodeInfo &node_info
    )  : ios_(ios),
         info_(node_info)
    {
    }

    void send_request(
        const string& req,
        function<string(const string&)> h,
        bool schedule_read);
};

#endif //BLUZELLE_PEER_H
