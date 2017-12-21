#ifndef BLUZELLE_PEERSERVER_H
#define BLUZELLE_PEERSERVER_H

#include <string>

using std::string;

#include <boost/asio.hpp>

#include "PeerListener.h"

class PeerServer {
private:
    boost::asio::io_service& ios_;
    boost::asio::ip::address ip_address_;
    unsigned short port_;
    std::function<string(const string&)> request_handler_;

    bool running_ = false;

public:
    PeerServer(boost::asio::io_service& ios,
               const string &ip_address,
               unsigned short port,
               std::function<string(const string&)> request_handler);

    void run();

    bool is_running() const;
};

#endif //BLUZELLE_PEERSERVER_H
