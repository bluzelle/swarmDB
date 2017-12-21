#ifndef BLUZELLE_WEBSOCKETSERVER_H
#define BLUZELLE_WEBSOCKETSERVER_H
#include "Listener.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>

class WebSocketServer
{
    boost::asio::ip::address address_;
    unsigned short port_;
    std::shared_ptr<Listener> listener_;
    size_t threads_;
    bool is_running_ = false;

public:
    WebSocketServer
            (
                    const char *ip_address,
                    unsigned short port,
                    const std::shared_ptr<Listener> &listener,
                    unsigned short threads
            )
            : address_(boost::asio::ip::address::from_string(ip_address)),
              port_(port),
              listener_(listener),
              threads_(threads)
    {
    }

    void operator()()
    {
        // The io_service is required for all I/O
        boost::asio::io_service ios
                {
                        static_cast<int>( threads_ )
                };

        // Create and launch a listening port
        listener_ = std::make_shared<Listener>
                (
                        ios,
                        tcp::endpoint{address_, port_}
                );
        listener_->run();

        // Run the I/O service on the requested number of threads
        std::vector<std::thread> v_;
        v_.reserve(threads_ - 1);
        for (auto i = threads_ - 1; i > 0; --i)
            v_.emplace_back(
                    [&ios]
                        {
                        ios.run();
                        });
        ios.run();
        is_running_ = false;
    }

    const bool &is_running() { return is_running_; }
};

#endif //BLUZELLE_WEBSOCKETSERVER_H
