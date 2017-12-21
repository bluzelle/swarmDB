#ifndef BLUZELLE_PEERLIST_H
#define BLUZELLE_PEERLIST_H

#include "Peer.h"
#include "NodeInfo.hpp"

#include <vector>

using std::vector;



class PeerList : public vector<Peer>{
public:
    PeerList(boost::asio::io_service& ios);

    /*NodeInfo& find_by_endpoint() const;

    NodeInfo& find_by_address() const;

    NodeInfo& find_by_name() const;*/
};

#endif //BLUZELLE_PEERLIST_H
