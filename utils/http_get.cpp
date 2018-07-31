// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License, version 3,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <include/bluzelle.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <regex>
#include <string>


namespace
{
    const std::regex url_regex{
        R"((?:http://)?)"      // Throw away http:// and www. if they're there
        R"((?:www\.)?)"
        R"(([^/\.]+\.[^/]+))"  // Domain must be something.something
        R"((/.*)?)"            // Target, if present, must be /something
    };
}


namespace bzn::utils::http
{
    // Performs an HTTP GET and returns the body of the HTTP response
    std::string sync_get(const std::string& url)
    {
        using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
        namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

        boost::asio::io_context ioc;
        tcp::resolver resolver{ioc};
        tcp::socket socket{ioc};

        std::smatch what;
        if (!std::regex_match(url, what, url_regex))
        {
            LOG(error) << "could not parse url " << url;
            throw std::runtime_error("could not parse url " + url);
        }

        const std::string host{what[1]};
        const std::string target{what[2]};

        auto const endpoint_iterator = resolver.resolve(host, "80");

        LOG(info) << "Connecting to: " << host << "...";

        boost::asio::connect(socket, endpoint_iterator);

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(socket, req);

        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;

        http::read(socket, buffer, res);

        socket.shutdown(tcp::socket::shutdown_both);

        return res.body();
    }

} // namespace bzn::utils::http
