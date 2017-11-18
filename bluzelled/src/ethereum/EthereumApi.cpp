#include "ethereum/EthereumApi.h"


double
EthereumApi::token_balance(const EthereumToken& t) {
    connect_socket();

    write_request(
            boost::str(boost::format(get_token_balance_by_token_contract_address_format) %
                       t.get_address() %
                       address_ % token));

    auto body = read_response();

    close_socket();

    auto tuple = parse_response(body);

    double balance = get_field<double>(
            tuple,
            "result");

    balance /= std::pow(10, t.get_decimal_points()); // Balance is stored in minimal denomination.

    return balance;
}

void
EthereumApi::connect_socket() {
    auto const results = resolver_.resolve(
            tcp::resolver::query(host_, "http"));
    boost::asio::connect(sock_, results);
}

void
EthereumApi::write_request(
        const string &target) {
    http::request <http::string_body> req{http::verb::get, target, 11/*version*/};

    req.set(http::field::host,
            host_);

    req.set(http::field::user_agent,
            BOOST_BEAST_VERSION_STRING);

    http::write(sock_,
                req);
}

string
EthereumApi::read_response() {
    boost::beast::flat_buffer buffer;
    http::response <http::string_body> res;

    http::read(sock_,
               buffer,
               res);

    return res.body();
}

void
EthereumApi::close_socket() {
    boost::system::error_code ec;
    sock_.shutdown(tcp::socket::shutdown_both,
                   ec);

    if (ec && ec != boost::system::errc::not_connected)
        throw boost::system::system_error{ec};
}

boost::property_tree::ptree
EthereumApi::parse_response(const string &body) const {
    if (!body.empty() && (body.front() == '{' && body.back() == '}'))
        {
        boost::property_tree::ptree tuple;
        std::stringstream ss;

        ss << body;

        boost::property_tree::read_json(ss,
                                        tuple);
        return tuple;
        }

    throw std::runtime_error("Failed to parse response: " + body);
}

template<typename T> T
EthereumApi::get_field(const boost::property_tree::ptree &tuple,
            const string &name) const {
    auto message_s = tuple.get<string>("message");

    if (message_s == "OK")
        {
        auto item = tuple.get<string>(name);
        return boost::lexical_cast<T>(item);
        }

    throw std::runtime_error(string("Faied to extract field '") + name + "'" + string(" from property tree"));
}

