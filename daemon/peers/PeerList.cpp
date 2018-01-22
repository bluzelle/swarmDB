#include "PeerList.h"
#include "node/DaemonInfo.h"

#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

PeerList::PeerList(
    boost::asio::io_service& ios
)
{
    DaemonInfo& daemon_info = DaemonInfo::get_instance();
    // Read file and create a list of known peers.
    // TODO Move this functionality into an init module.
    boost::filesystem::ifstream file("./peers");

    string peer_info; // name=Host:port [todo] Pick a format for node info storage.
    while (getline(file, peer_info))
        {
        if (peer_info.front() != ';') // Skip commented lines.
            {
            NodeInfo n;
            n.name() = peer_info.substr(0, peer_info.find('='));
            n.host() =  peer_info.substr(peer_info.find('=') + 1, peer_info.find(':') - peer_info.find('=') - 1);
            n.port() = boost::lexical_cast<uint16_t>(peer_info.substr(peer_info.find(':') + 1));

            if( (n.name() != daemon_info.host_ip()) && (n.port() != daemon_info.host_port()) )
                {
                peers_.emplace_back(Peer(ios,n));
                }
            }
        }
}
