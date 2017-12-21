#ifndef BLUZELLE_PEERSESSION_H
#define BLUZELLE_PEERSESSION_H

#include <string>

using std::string;

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>



class PeerSession : public std::enable_shared_from_this<PeerSession> {
//private:
public:
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    boost::asio::io_service::strand strand_;
    boost::beast::multi_buffer buffer_;
    std::function<string(const string&)> handler_ = nullptr;

public:
    explicit PeerSession(boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws);

    void run();

    void set_request_handler(std::function<string(const string&)> request_handler);

    void on_accept(boost::system::error_code ec);

    void do_read();

    void on_read(boost::system::error_code ec,
            std::size_t bytes_transferred);

    void on_write(boost::system::error_code ec,
                  std::size_t bytes_transferred,
                  bool schedule_read = true);

    void write_async(const string& message);
};

#endif //BLUZELLE_PEERSESSION_H
