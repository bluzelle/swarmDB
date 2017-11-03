#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "BaseClassModule"

#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "EthereumApi.h"

//BOOST_AUTO_TEST_CASE( constructors )
//        {
//                BOOST_CHECK(false);
//        }


BOOST_AUTO_TEST_CASE( parse_http_response )
    {
        string responseText = "HTTP/1.1 200 OK\r\nDate: Wed, 11 Oct 2017 18:14:00 GMT\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: 292\r\nConnection: close\r\nSet-Cookie: __cfduid=d17f6323ca4a23f6ad1c5d25ab36492441507745640; expires=Thu, 11-Oct-18 18:14:00 GMT; path=/; domain=.typicode.com; HttpOnly\r\nX-Powered-By: Express\r\nVary: Origin, Accept-Encoding\r\nAccess-Control-Allow-Credentials: true\r\nCache-Control: public, max-age=14400\r\nPragma: no-cache\r\nExpires: Wed, 11 Oct 2017 22:14:00 GMT\r\nX-Content-Type-Options: nosniff\r\nEtag: W/\"124-yiKdLzqO5gfBrJFrcdJ8Yq0LGnU\"\r\nVia: 1.1 vegur\r\nCF-Cache-Status: HIT\r\nServer: cloudflare-nginx\r\nCF-RAY: 3ac3cbef76e61bb5-SEA\r\n\r\n{\n  \"userId\": 1,\n  \"id\": 1,\n  \"title\": \"sunt aut facere repellat provident occaecati excepturi optio reprehenderit\",\n  \"body\": \"quia et suscipit\\nsuscipit recusandae consequuntur expedita et cum\\nreprehenderit molestiae ut ut quas totam\\nnostrum rerum est autem sunt rem eveniet architecto\"\n}";
        HttpResponse r(responseText);

        BOOST_CHECK_EQUAL(r.getStatus(), 200);
        BOOST_CHECK_EQUAL(r.getContentLength(), 292);

        auto s = r.getContent();
        BOOST_CHECK(!s.empty());
        BOOST_CHECK_EQUAL(s.length(), r.getContentLength());

        // Payload is JSON.
        BOOST_CHECK_EQUAL(s.front(), '{');
        BOOST_CHECK_EQUAL(s.back(), '}');
    }

// Sends GET to http://jsonplaceholder.typicode.com/posts/1
// and parses JSON response.
BOOST_AUTO_TEST_CASE( http_get )
    {
        const string httpGetFormat =
                "GET %s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nConnection: close\r\n\r\n";

        string h = "jsonplaceholder.typicode.com";

        auto request = boost::format(httpGetFormat) % "/posts/1" % h;
        auto response = HttpRequest::send(h, boost::str(request));

        HttpResponse r(response);

        auto status = r.getStatus();
        auto length = r.getContentLength();
        auto b = r.getContent();

        boost::property_tree::ptree tuple;
        std::stringstream ss;
        ss << b;
        boost::property_tree::read_json(ss, tuple);

        auto t = tuple.get<string>("title");

        BOOST_CHECK(!t.empty());
    }

// Testing coin balance using etherscan.io API.
BOOST_AUTO_TEST_CASE( ethereum_api )
    {
        EthereumApi* account_one =   new EthereumApi("0xe04f27eb70e025b78871a2ad7eabe85e61212761");
        EthereumApi* account_two = new EthereumApi("0xce7468901c428d8004bd4118c4d14239a6231afe");

        const string Lunyr("0xfa05A73FfE78ef8f1a739473e462c54bae6567D9"); // Lunyr token contract address.

        // Check Lunyr token balance on both accounts, one should have 0 and another non-zero balance.
        auto lunyr_balance_one = account_one->tokenBalance(Lunyr);
        auto lunyr_balance_two = account_two->tokenBalance(Lunyr);

        BOOST_CHECK(lunyr_balance_one != lunyr_balance_two);
    }
