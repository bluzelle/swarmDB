#include <thread>
#include <memory>

#include "PeerServer.h"

PeerServer::PeerServer(boost::asio::io_service& ios,
                       const string &ip_address,
                       unsigned short port,
                       unsigned short threads,
                       std::function<string(const string&)> request_handler)
: ios_(ios),
  ip_address_(boost::asio::ip::address::from_string(ip_address)),
    port_(port),
    threads_(threads),
    request_handler_(request_handler) {

}

void PeerServer::run() {
    auto listener = std::make_shared<PeerListener>(ios_,
                                              boost::asio::ip::tcp::endpoint{ip_address_, port_},
                                              request_handler_);
    listener->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v_;
    v_.reserve(threads_ - 1);
    for (auto i = threads_ - 1; i > 0; --i)
        v_.emplace_back(
                [this]
                    {
                    ios_.run();
                    });

    running_ = true;
}

bool PeerServer::is_running() const {
    return running_;
}


