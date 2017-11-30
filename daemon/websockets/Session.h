#ifndef BLUZELLE_SESSION_H
#define BLUZELLE_SESSION_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>
namespace pt = boost::property_tree;

void
fail(boost::system::error_code ec, char const *what);

class Session : public std::enable_shared_from_this<Session>
{
    websocket::stream<tcp::socket> ws_;
    boost::asio::io_service::strand strand_;
    boost::beast::multi_buffer buffer_;
    size_t seq_ = 0;

    std::string
    fix_json_numbers(const std::string &json_str)
    {
        boost::regex re(R"(\"([0-9]+\.{0,1}[0-9]*)\")");
        return  boost::regex_replace(json_str, re, "$1");
    };

    std::string
    process_json_string(
            const boost::beast::multi_buffer &buffer
    );

    std::string
    update_nodes();

public:
    explicit
    Session(tcp::socket socket); // Take ownership of the socket

    void
    run();

    void
    on_accept(boost::system::error_code ec);

    void
    do_read();

    void
    on_read(
            boost::system::error_code ec,
            std::size_t bytes_transferred);

    void
    on_write(
            boost::system::error_code ec,
            std::size_t bytes_transferred);

    std::string
    timestamp();
};

#endif //BLUZELLE_SESSION_H
