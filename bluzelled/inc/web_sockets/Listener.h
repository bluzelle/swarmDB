#ifndef KEPLER_LISTENER_H
#define KEPLER_LISTENER_H

#include "web_sockets/Session.h"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

void
fail(boost::system::error_code ec, char const *what);

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener> {
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    std::vector<std::weak_ptr<Session>> sessions_;

public:
    Listener(
            boost::asio::io_service &ios,
            tcp::endpoint endpoint)
            : acceptor_(ios), socket_(ios) {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
            {
            fail(ec, "open");
            return;
            }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec)
            {
            fail(ec, "bind");
            return;
            }

        // Start listening for connections
        acceptor_.listen(
                boost::asio::socket_base::max_connections, ec);
        if (ec)
            {
            fail(ec, "listen");
            return;
            }
    }

    // Start accepting incoming connections
    void
    run() {
        if (!acceptor_.is_open())
            return;
        do_accept();
    }

    void
    do_accept() {
        acceptor_.async_accept(
                socket_,
                std::bind(
                        &Listener::on_accept,
                        shared_from_this(),
                        std::placeholders::_1));
    }

    void
    on_accept(boost::system::error_code ec) {
        if (ec)
            {
            fail(ec, "accept");
            }
        else
            {
            // Create the session and run it
            auto s = std::make_shared<Session>(std::move(socket_));
            s->run();
            sessions_.push_back(s);
            }

        // Accept another connection
        do_accept();
    }
};


#endif //KEPLER_LISTENER_H
