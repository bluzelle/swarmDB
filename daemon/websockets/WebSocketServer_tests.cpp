#define BOOST_TEST_DYN_LINK

#include "Listener.h"
#include "WebSocketServer.h"

#include <boost/test/unit_test.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <string>
#include <algorithm>

std::string connect_send_and_receive(
        boost::asio::ip::tcp::socket &sock,
        const std::string &host,
        unsigned long port,
        const std::string &message);

struct F
{
    F() = default;

    ~F() = default;
};

void
remove_whitespace(std::string &text);

// --run_test=websockets
BOOST_FIXTURE_TEST_SUITE(websockets, F)

    BOOST_AUTO_TEST_CASE(echo_ping)
    {
        try
            {
            namespace websocket = boost::beast::websocket;
            const char *host = "127.0.0.1";
            const unsigned int port = 54000;

            std::shared_ptr<Listener> listener;
            WebSocketServer sut(
                    host,
                    port,
                    listener,
                    1);
            boost::thread websocket_server_thread = boost::thread(sut);

            boost::asio::io_service ios;
            boost::asio::ip::tcp::socket sock(ios);

            std::string message = R"({"cmd":"ping","seq":1234})";
            std::string accepted_response = R"({"cmd":"pong","seq":"1234"})";
            std::string response = connect_send_and_receive(sock, host, port, message);
            remove_whitespace(response);

            BOOST_CHECK_EQUAL(accepted_response, response);

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(2s);

            sock.close();
            }
        catch (const std::exception &e)
            {
            std::cout << "Caught a standard exception:: [" << e.what() << "]" << std::endl;
            BOOST_ASSERT(fail);
            }
        catch (...)
            {
            std::cerr << "Caught an unhandled exception." << std::endl;
            BOOST_ASSERT(fail);
            }
    }

BOOST_AUTO_TEST_SUITE_END()


void
remove_whitespace(
        std::string &text)
{
    text.erase(
            std::remove_if(
                    text.begin(),
                    text.end(),
                    [](unsigned char x)
                        { return std::isspace(x); }),
            text.end()
    );
}

std::string
connect_send_and_receive(
        boost::asio::ip::tcp::socket &sock,
        const std::string &host,
        unsigned long port,
        const std::string &message)
{
    boost::asio::ip::tcp::resolver r(sock.get_io_service());
    boost::asio::connect(sock, r.resolve(boost::asio::ip::tcp::resolver::query{host, std::to_string(port)}));
    using namespace boost::beast::websocket;

    stream<boost::asio::ip::tcp::socket &> ws(sock);
    ws.handshake(host, "/");
    boost::asio::streambuf sb;
    ws.read(sb);
    sb.consume(sb.size());

    // Now we can send our ping request
    ws.write(boost::asio::buffer(message));

    //boost::asio::streambuf sb;
    ws.read(sb);

    char close_reason[256] = {0};
    ws.close(close_reason);

    return std::string((std::istreambuf_iterator<char>(&sb)), std::istreambuf_iterator<char>());
}
