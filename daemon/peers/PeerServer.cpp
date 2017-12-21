#include "PeerServer.h"

#include <thread>
#include <memory>
#include <iostream>

PeerServer::PeerServer(boost::asio::io_service& ios,
                       const string &ip_address,
                       unsigned short port,
                       std::function<string(const string&)> request_handler)
: ios_(ios),
    ip_address_(boost::asio::ip::address::from_string(ip_address)),
    port_(port),
    request_handler_(request_handler)
{

}

void PeerServer::run()
{
    auto listener = std::make_shared<PeerListener>(ios_,
                                              boost::asio::ip::tcp::endpoint{ip_address_, port_},
                                              request_handler_);
    listener->run();
    running_ = true;
}

bool PeerServer::is_running() const
{
    return running_;
}


