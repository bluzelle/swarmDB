#include <cstdarg>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

using namespace boost::asio;


#include "EthereumApi.h"


double EthereumApi::tokenBalance(const string& tokenContractAddress) {
    auto rest = boost::format(getTokenBalanceByTokenContractAddressFormat) % tokenContractAddress % address;
    auto request = boost::format(httpGetFormat) % rest % host;

    auto response = sendRequest(host, boost::str(request));

    // Extract HTTP status.
    int status = 200;
    /*const string statusPattern = "^HTTP/\\d\\.\\d (\\d{3} .+)$";
    boost::regex regexStatus(statusPattern);
    boost::sregex_iterator it_s(statusPattern.begin(), statusPattern.end(), regexStatus);
    boost::sregex_iterator end;

    for (; it_s != end; ++it_s) {
        status = boost::lexical_cast<int>(it_s->str());
    }*/

    if (status == 200) {
        // Get Content-Length
        int length = 42;
        /*const string lengthPattern = "^Content-Length: (\\d+)$";
        boost::regex regexContentLength(lengthPattern);
        boost::sregex_iterator it_l(lengthPattern.begin(), lengthPattern.end(), regexContentLength);

        for (; it_l != end; ++it_l) {
            length = boost::lexical_cast<int>(it_l->str());
        }*/

        // Get JSON payload.
        string content = response.substr(response.length()-length, length);

        boost::property_tree::ptree tuple;
        std::stringstream ss;
        ss << content;
        boost::property_tree::read_json(ss, tuple);

        return boost::lexical_cast<double>(tuple.get<string>("result"));
    }

    return -1.0; // Negative balance indicates error.
}

string EthereumApi::sendRequest(const string& host, const string& request) {
    io_service s;

    ip::tcp::resolver::query query(host, "http");
    ip::tcp::resolver r(s);
    auto it = r.resolve(query);
    ip::tcp::resolver::iterator end;

    ip::tcp::socket socket(s);
    boost::system::error_code err = boost::asio::error::host_not_found;

    while (err && it != end) {
        socket.close();
        socket.connect(*it++, err);
    }

    socket.send(boost::asio::buffer(request));

    string response;
    boost::array<char, 1024> buf;

    do {
        size_t transferred = socket.receive(boost::asio::buffer(buf), {}, err);
        if (!err)
            response.append(buf.begin(), buf.begin() + transferred);
    } while (!err);

    return response;
}

double EthereumApi::restTest() {
    string h = "jsonplaceholder.typicode.com";
    auto request =    boost::format(httpGetFormat) % "/posts/1" % h;

    auto response = sendRequest(h, boost::str(request));

    boost::property_tree::ptree tuple;
    boost::property_tree::read_json(response, tuple);

    auto n = tuple.get_value("name");

    return 1.0;
}

