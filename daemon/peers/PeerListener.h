#ifndef BLUZELLE_PEERLISTENER_H
#define BLUZELLE_PEERLISTENER_H

#include <string>

using std::string;

#include <boost/asio/ip/tcp.hpp>

class PeerListener : public std::enable_shared_from_this<PeerListener> {
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::socket socket_;
    std::function<string(const string&)> request_handler_;

public:
    PeerListener(boost::asio::io_service &ios,
                 boost::asio::ip::tcp::endpoint endpoint,
                 std::function<string(const string&)> handler);

    void run();

    void do_accept();

    void on_accept(boost::system::error_code ec);
};

#endif //BLUZELLE_PEERLISTENER_H
