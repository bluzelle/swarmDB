#include "Peer.h"
#include "DaemonInfo.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/beast/websocket.hpp>

using namespace std;

void
Peer::send_request(
    const string& req,
    function<string(const string&)> h,
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
                    info_.host(),
                    boost::lexical_cast<string>(info_.port())
                }
        );
        boost::asio::connect(ws.next_layer(), lookup);
        ws.handshake(
            info_.host(),
            "/");

        session = make_shared<PeerSession>(
            move(ws)
        ); // Store it for future use.

        session->set_request_handler(h);
        session->schedule_read_ = schedule_read;
        }
    catch
        (
            exception& ex
        )
        {
        cout << "Cannot connect to "
             << info_.host() << ":"
             << info_.port() << " '"
             << ex.what() << "'\n";
        }
    catch (...)
        {
        cout << "Unhandled exception." << endl;
        }

    if (session != nullptr)
        {
        session->schedule_read_ = schedule_read;
        session->write_async(req);
        }
    else
        {
        cout << "No session to send request to" << endl;
        }
}
