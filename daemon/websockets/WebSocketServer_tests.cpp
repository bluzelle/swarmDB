#define BOOST_TEST_DYN_LINK

#include "Listener.h"
#include "WebSocketServer.h"

#include <boost/test/unit_test.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

struct F
{
    F() = default;

    ~F() = default;
};

// --run_test=websockets
BOOST_FIXTURE_TEST_SUITE(websockets, F)
    BOOST_AUTO_TEST_CASE(echo)
    {

        try
            {
            using tcp = boost::asio::ip::tcp;
            namespace websocket = boost::beast::websocket;

            const char *host = "127.0.0.1";
            const unsigned int port = 3000;

            std::shared_ptr<Listener> listener;
            WebSocketServer web_socket_server(
                    host,
                    port,
                    listener,
                    1);
            boost::thread websocket_server_thread = boost::thread(web_socket_server);
            std::cout << "Websocket started" << std::endl;


            // Normal boost::asio setup

            boost::asio::io_service ios;
            boost::asio::ip::tcp::resolver r(ios);
            boost::asio::ip::tcp::socket sock(ios);
            boost::asio::connect(sock, r.resolve(boost::asio::ip::tcp::resolver::query{host, "3000"}));

            using namespace boost::beast::websocket;

            // WebSocket connect and send message using beast
            stream<boost::asio::ip::tcp::socket&> ws(sock);
            ws.handshake(host, "/");
            ws.write(boost::asio::buffer(R"({"cmd":"start", "seq":123})"));

            // Receive WebSocket message, print and close using beast
//        boost::asio::streambuf sb;
//        opcode op;
//
//        ws.read(op, sb);
//        ws.close(close_code::normal);
//        std::cout <<
//                  boost::beast::debug::buffers_to_string(sb.data()) << "\n";


            }
        catch(const std::exception &e)
            {
            std::cout << "Caught a standard exception:: [" << e.what() << "]" << std::endl;
            }
        catch(...)
            {
            std::cerr << "Caught an unhandled exception." << std::endl;
            }




    }
BOOST_AUTO_TEST_SUITE_END()

