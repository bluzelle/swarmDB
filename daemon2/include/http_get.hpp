#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/regex.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include <include/bluzelle.hpp>

namespace
{
    boost::regex url_regex{
        R"((?:http://)?)"      // Throw away http:// and www. if they're there
        R"((?:www\.)?)"
        R"(([^/\.]+\.[^/]+))"  // Domain must be something.something
        R"((/.*)?)"            // Target, if present, must be /something
    };
}

namespace bzn::http
{
	using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
	namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

	// Performs an HTTP GET and returns the body of the HTTP response
    std::string sync_get(const std::string& url)
	{
		boost::asio::io_context ioc;
		tcp::resolver resolver{ioc};
		tcp::socket socket{ioc};

        boost::smatch what;
        if(!boost::regex_match(url, what, url_regex)){
            LOG(error) << "could not parse url " << url;
            throw std::runtime_error("could not parse url " + url);
        }

        std::string host = what[1];
        std::string target = what[2];

		auto const results = resolver.resolve(host, "80");
		boost::asio::connect(socket, results.begin(), results.end());

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
}
