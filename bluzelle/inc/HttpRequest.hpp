#ifndef KEPLER_HTTPREQUEST_H
#define KEPLER_HTTPREQUEST_H

#include <string>

#include <boost/array.hpp>
#include <boost/asio.hpp>

using namespace boost::asio;

using std::string;

class HttpRequest {
public:
    static string send(const string& host, const string& request) {
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
};

#endif //KEPLER_HTTPREQUEST_H
