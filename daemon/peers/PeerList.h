#ifndef BLUZELLE_PEERLIST_H
#define BLUZELLE_PEERLIST_H

#include "Peer.h"
#include "NodeInfo.hpp"

#include <vector>

using namespace std;

class PeerList {
    vector<Peer> peers_;
public:
    PeerList(
        boost::asio::io_service& ios
    );

    vector<Peer>&
    peers()
    {
        return peers_;
    }
};

#endif //BLUZELLE_PEERLIST_H
