// Copyright (C) 2019 Bluzelle
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
#include <utils/utils_interface.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/rfc2818_verification.hpp>
#include <regex>
#include <string>


namespace
{
    const std::regex URL_REGEX("(http|https)://([^/ :]+):?([^/ ]*)(/.*)?");

    const std::string CERT_DIRS[] = {
            "/system/etc/security/cacerts", // Android
            "/usr/local/share/certs",       // FreeBSD
            "/etc/pki/tls/certs",           // Fedora/RHEL
            "/etc/ssl/certs",               // Ubuntu
            "/etc/openssl/certs",           // NetBSD
            "/var/ssl/certs",               // AIX
    };
}


namespace bzn
{
    // Performs an HTTP GET or POST and returns the body of the HTTP response...
    std::string
    utils_interface::sync_req(const std::string& url, const std::string& post) const
    {
        using tcp = boost::asio::ip::tcp;
        namespace ssl = boost::asio::ssl;
        namespace http = boost::beast::http;

        boost::asio::io_context ioc;
        tcp::resolver resolver{ioc};

        std::smatch what;
        if (!std::regex_match(url, what, URL_REGEX))
        {
            LOG(error) << "could not parse url " << url;
            throw std::runtime_error("could not parse url " + url);
        }

        const std::string protocol{what[1]};
        const std::string host{what[2]};
        const std::string path{what[4]};
        std::string port{what[3]};

        // secure connection?
        const bool secure = (protocol == "https");

        // if no port is specified then default to 80 or 443...
        if (port.empty())
        {
            port = protocol;
        }

        auto const endpoint_iterator = resolver.resolve(host, port);

        LOG(info) << "Connecting to: " << host << "...";

        ssl::context ctx(boost::asio::ssl::context::sslv23_client);
        ssl::stream<tcp::socket> ssl_stream(ioc, ctx);
        tcp::socket socket{ioc};

        http::request<http::string_body> req{http::verb::get, (!path.empty()) ? path : "/", 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // post or get?
        if (!post.empty())
        {
            req.method(http::verb::post);
            req.body() = post;
            req.prepare_payload();
        }

        if (secure)
        {
            // set default paths for finding CA certificates...
            ctx.set_default_verify_paths();

            // other possible locations...
            for(const auto& cert_path : CERT_DIRS)
            {
                ctx.add_verify_path(cert_path);
            }

            // Set SNI Hostname (many hosts need this to handshake successfully)
            if (!SSL_set_tlsext_host_name(ssl_stream.native_handle(), host.c_str()))
            {
                boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::beast::net::error::get_ssl_category()};
                throw boost::beast::system_error{ec};
            }

            boost::asio::connect(ssl_stream.next_layer(), endpoint_iterator);

            ssl_stream.set_verify_mode(ssl::verify_peer);
            ssl_stream.set_verify_callback(ssl::rfc2818_verification(host));

            ssl_stream.handshake(ssl::stream_base::client);
            http::write(ssl_stream, req);
        }
        else
        {
            boost::asio::connect(socket, endpoint_iterator);
            http::write(socket, req);
        }

        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;

        if (secure)
        {
            http::read(ssl_stream, buffer, res);
            boost::system::error_code ec;
            ssl_stream.shutdown(ec);
        }
        else
        {
            http::read(socket, buffer, res);
            socket.shutdown(tcp::socket::shutdown_both);
        }

        return res.body();
    }

} // namespace bzn::utils::http