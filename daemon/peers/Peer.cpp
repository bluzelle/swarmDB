#include "Peer.h"
#include "DaemonInfo.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/beast/websocket.hpp>

using std::shared_ptr;


void Peer::send_request(const string& req,
                        std::function<string(const string&)> h,
                        bool schedule_read)
{
    shared_ptr<PeerSession> session;
    try
        {
        boost::asio::ip::tcp::resolver resolver(ios_);
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws{ios_};

        // Resolve, connect, handshake.
        auto lookup = resolver.resolve(
                {
                        info_.get_value<std::string>("host"),
                        info_.get_value<string>("port")
                }
        );
        boost::asio::connect(ws.next_layer(), lookup);
        ws.handshake(info_.get_value<string>("host"), "/");

        session = std::make_shared<PeerSession>(std::move(ws)); // Store it for future use.

        session->set_request_handler(h);
        session->schedule_read_ = schedule_read;
        }
    catch (std::exception& ex)
        {
        std::cout << "Cannot connect to "
                  << info_.get_value<string>("host") << ":"
                  << info_.get_value<string>("port") << " '"
                  << ex.what() << "'" << std::endl;
        }
    catch (...)
        {
        std::cout << "Unhandled exception." << std::endl;
        }

    if (session != nullptr)
        {
        session->schedule_read_ = schedule_read;
        session->write_async(req);
        }
    else
        {
        std::cout << "No session to send request to" << std::endl;
        }
}
