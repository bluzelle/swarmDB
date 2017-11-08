#ifndef KEPLER_ETHEREUM_API_H_H
#define KEPLER_ETHEREUM_API_H_H



#include <string>
#include <iostream>

#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>


using std::string;

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

class ethereum_token {
public:
    string address_;
    unsigned int decimal_points_;

    ethereum_token(string a, unsigned int d)
            : address_(a), decimal_points_(d) { }
};

class ethereum_api {
protected:
    const string host_ = "ropsten.etherscan.io";

    const string getTokenBalanceByTokenContractAddressFormat =
            "/api?module=account&action=tokenbalance&contractaddress=%s&address=%s&tag=latest&apikey=YourApiKeyToken";

    string address_;

    tcp::resolver resolver_;
    tcp::socket sock_;

public:
    ethereum_api(string addr, boost::asio::io_service& ios)
            : address_(std::move(addr)), resolver_(ios), sock_(ios) {

    }

    double token_balance(ethereum_token t) {
        tcp::resolver::query query(host_, "http");
        auto const results = resolver_.resolve(query);

        boost::asio::connect(sock_, results);

        auto target = boost::str(boost::format(getTokenBalanceByTokenContractAddressFormat) % t.address_ % address_);

        http::request<http::string_body> req{http::verb::get, target, 11/*version*/};
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(sock_, req);

        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;

        http::read(sock_, buffer, res);

        // Close the socket
        boost::system::error_code ec;
        sock_.shutdown(tcp::socket::shutdown_both, ec);

        if (ec && ec != boost::system::errc::not_connected)
            throw boost::system::system_error{ec};

        auto body = res.body();
        if (!body.empty() && (body.front() == '{' && body.back() == '}'))
            {
            boost::property_tree::ptree tuple;
            std::stringstream ss;
            ss << body;
            boost::property_tree::read_json(ss, tuple);

            auto result_s = tuple.get<string>("result");
            auto message_s = tuple.get<string>("message");
            if (message_s == "OK")
                {
                double balance = boost::lexical_cast<double>(result_s) / std::pow(10, t.decimal_points_) ;
                return balance;
                }
            }

        return -1.0; // Error.
    }
};


#endif //KEPLER_ETHEREUM_API_H_H
