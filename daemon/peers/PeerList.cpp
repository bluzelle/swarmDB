#include <boost/lexical_cast.hpp>

#include "PeerList.h"

PeerList::PeerList(boost::asio::io_service& ios) {
    // Hardcoded list of peers.
    ushort leader_port =  58000; // One node starts on port 58000 and become leader.
    for (int i = 1; i <= 2; ++i) // Create 5 more followers.
        {
        NodeInfo n;
        n.port_+= i;
        n.name_+= "_node_port_" + boost::lexical_cast<string>(n.port_); // Names must be unique becayse they usd to name storage file.
        this->emplace_back(ios, n);
        }
}
