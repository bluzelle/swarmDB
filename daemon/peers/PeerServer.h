#pragma once

#include "PeerListener.h"
#include <boost/asio.hpp>
#include <string>


class PeerServer
{
public:
    // todo: this should move to the listener?
    using request_handler_t = std::function<std::string(const std::string&)>;

    /**
     * Constructor
     * @param ios io service
     * @param ip_address address to listen on
     * @param port port to accept connections on
     * @param request_handler request handler
     */
    PeerServer(boost::asio::io_service& ios,
               const std::string& ip_address,
               uint16_t port,
               request_handler_t request_handler);

    /**
     * Start the server
     */
    void run();

    /**
     * Server status
     * @return return true if running
     */
    bool is_running() const;

private:
    boost::asio::io_service& ios;
    boost::asio::ip::address ip_address;
    uint16_t port;
    request_handler_t request_handler;
    bool running = false;
};
