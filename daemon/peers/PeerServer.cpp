#include "PeerServer.h"


PeerServer::PeerServer(boost::asio::io_service& ios,
                       const std::string& ip_address,
                       uint16_t port,
                       request_handler_t request_handler)
    : ios(ios)
    , ip_address(boost::asio::ip::address::from_string(ip_address))
    , port(port)
    , request_handler(std::move(request_handler))
{
}


void
PeerServer::run()
{
    auto listener = std::make_shared<PeerListener>(
            this->ios, boost::asio::ip::tcp::endpoint{this->ip_address, this->port}, this->request_handler);

    listener->run();

    this->running = true;
}


bool
PeerServer::is_running() const
{
    return this->running;
}
