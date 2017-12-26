#include "PeerList.h"
#include "node/DaemonInfo.h"

#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

PeerList::PeerList(boost::asio::io_service& ios)
{
    // Hardcoded list of peers.
    DaemonInfo& daemon_info = DaemonInfo::get_instance();
    ushort port = daemon_info.get_value<int>("port");
    string host = boost::asio::ip::address_v4::loopback().to_string();

    // Read file and create a list of known peers.
    boost::filesystem::ifstream file("./peers");

    string peer_info; // name=Host:port [todo] Pick a format for node info storage.
    while (getline(file, peer_info))
        {
        if (peer_info.front() != ';') // Skip commented lines.
            {
            NodeInfo n;
            n.set_value("name", peer_info.substr(0, peer_info.find('=')));
            n.set_value("host", peer_info.substr(peer_info.find('=') + 1, peer_info.find(':') - peer_info.find('=') - 1));
            n.set_value("port", peer_info.substr(peer_info.find(':') + 1));
            if (host != n.get_value<string>("host") // Exclude this node.
                && port != n.get_value<ushort>("port"))
                this->emplace_back(ios, n);
            }
        }
}
