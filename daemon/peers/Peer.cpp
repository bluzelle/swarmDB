#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/beast/websocket.hpp>

#include "Peer.h"

using std::shared_ptr;

Peer::Peer(boost::asio::io_service& ios, NodeInfo i) : info_(std::move(i)), ios_(ios) {

}

string Peer::send_request(const string& req) {

    if (session_ == nullptr) // Check if we don't have a session.
        {
        try
            {
            boost::asio::ip::tcp::resolver resolver(ios_);
            boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws{ios_};

            // Resolve, connect, handshake.
            auto lookup = resolver.resolve({info_.host_, boost::lexical_cast<string>(info_.port_)});
            boost::asio::connect(ws.next_layer(), lookup);

            ws.handshake(info_.host_, "/");

            session_ = std::make_shared<PeerSession>(std::move(ws)); // Store it for future use.
            }
        catch (std::exception& ex)
            {
            std::cout << "Cannot connect to "
                      << info_.host_ << ":"
                      << boost::lexical_cast<string>(info_.port_) << " '"
                      << ex.what() << "'" << std::endl;
            }
        }

    if (session_ != nullptr) // Check again in case we just created a session.
        {
        session_->write_async(req);
        }
    else
        {
        std::cout << "No session to send request to" << std::endl;
        }
}

/*string Peer::handle_response(const string& resp) {
    std::cout << "<<" << resp << std::endl;
}*/