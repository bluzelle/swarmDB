#include "PeerList.h"
#include "node/DaemonInfo.h"

#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>

PeerList::PeerList(boost::asio::io_service& ios)
{
    // Hardcoded list of peers.
    DaemonInfo& daemon_info = DaemonInfo::get_instance();
    ushort leader_port = daemon_info.get_value<int>("port");
    int other_port = leader_port;

    NodeInfo n;
    for (int i = 0; i < 5; ++i) // Create 5 more followers.
        {
        other_port++;
        n.set_value("port", other_port);
        n.set_value("name", "_node_port_" + boost::lexical_cast<string>(other_port));
        this->emplace_back(ios, n);
        }
}
