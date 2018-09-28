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
#include <node/node.hpp>
#include <node/session.hpp>

using namespace bzn;

namespace
{
    const std::string BZN_API_KEY = "bzn-api";
}


node::node(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::beast::websocket_base> websocket, std::shared_ptr<chaos_base> chaos, const std::chrono::milliseconds& ws_idle_timeout,
    const boost::asio::ip::tcp::endpoint& ep)
    : tcp_acceptor(io_context->make_unique_tcp_acceptor(ep))
    , io_context(std::move(io_context))
    , websocket(std::move(websocket))
    , chaos(std::move(chaos))
    , ws_idle_timeout(ws_idle_timeout)
{
}


void
node::start()
{
    std::call_once(this->start_once, &node::do_accept, this);
}


bool
node::register_for_message(const std::string& msg_type, bzn::message_handler msg_handler)
{
    std::lock_guard<std::mutex> lock(this->message_map_mutex);

    // never allow!
    if (!msg_handler)
    {
        return false;
    }

    if (this->message_map.find(msg_type) != this->message_map.end())
    {
        LOG(debug) << msg_type << " message type already registered";

        return false;
    }

    this->message_map[msg_type] = std::move(msg_handler);

    return true;
}


bool
node::register_for_message(const bzn_msg_type type, bzn::protobuf_handler msg_handler)
{
    std::lock_guard<std::mutex> lock(this->message_map_mutex);

    // never allow!
    if (!msg_handler)
    {
        return false;
    }

    if (this->protobuf_map.find(type) != this->protobuf_map.end())
    {
        LOG(debug) << bzn_msg_type_Name(type) << " message type already registered";

        return false;
    }

    this->protobuf_map[type] = std::move(msg_handler);

    return true;
}


void
node::do_accept()
{
    this->acceptor_socket = this->io_context->make_unique_tcp_socket();

    this->tcp_acceptor->async_accept(*this->acceptor_socket,
        [self = shared_from_this()](const boost::system::error_code& ec)
        {
            if (ec)
            {
                LOG(error) << "accept failed: " << ec.message();
            }
            else
            {
                auto ep = self->acceptor_socket->remote_endpoint();

                LOG(debug) << "connection from: " << ep.address() << ":" << ep.port();

                auto ws = self->websocket->make_unique_websocket_stream(
                    self->acceptor_socket->get_tcp_socket());

                std::make_shared<bzn::session>(self->io_context, ++self->session_id_counter, std::move(ws), self->chaos, self->ws_idle_timeout)->start(
                        std::bind(&node::priv_msg_handler, self, std::placeholders::_1, std::placeholders::_2),
                        std::bind(&node::priv_protobuf_handler, self, std::placeholders::_1, std::placeholders::_2));
            }

            self->do_accept();
        });
}


void
node::priv_msg_handler(const Json::Value& msg, std::shared_ptr<bzn::session_base> session)
{
    if (msg.isMember(BZN_API_KEY))
    {
        std::lock_guard<std::mutex> lock(this->message_map_mutex);

        if (auto it = this->message_map.find(msg[BZN_API_KEY].asString()); it != this->message_map.end())
        {
            it->second(msg, std::move(session));
            return;
        }
    }

    LOG(debug) << "no handler for:\n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

    session->close();
}

void
node::priv_protobuf_handler(const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
{
    std::lock_guard<std::mutex> lock(this->message_map_mutex);

    if (auto it = this->protobuf_map.find(msg.type()); it != this->protobuf_map.end())
    {
        // Should also do signature verification here
        it->second(msg, std::move(session));
    }
    else
    {
        LOG(debug) << "no handler for message type " << bzn_msg_type_Name(msg.type());
    }

}

void
node::send_message_str(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::encoded_message> msg)
{
    if (this->chaos->is_message_delayed())
    {
        const boost::asio::ip::tcp::endpoint ep_copy = ep;
        this->chaos->reschedule_message(std::bind(&node::send_message, shared_from_this(), std::move(ep_copy), std::move(msg)));
        return;
    }

    if (this->chaos->is_message_dropped())
    {
        return;
    }

    std::shared_ptr<bzn::asio::tcp_socket_base> socket = this->io_context->make_unique_tcp_socket();

    socket->async_connect(ep,
            [self = shared_from_this(), socket, ep, msg](const boost::system::error_code& ec)
            {
                if (ec)
                {
                    LOG(error) << "failed to connect to: " << ep.address().to_string() << ":" << ep.port() << " - " << ec.message();

                    return;
                }

                // we've completed the handshake...
                std::shared_ptr<bzn::beast::websocket_stream_base> ws = self->websocket->make_unique_websocket_stream(socket->get_tcp_socket());

                ws->async_handshake(ep.address().to_string(), "/",
                        [self, ws, msg](const boost::system::error_code& ec)
                        {
                            if (ec)
                            {
                                LOG(error) << "handshake failed: " << ec.message();

                                return;
                            }

                            auto session = std::make_Shared<bzn::session>(self->io_context, ++self->session_id_counter, ws, self->chaos, self->ws_idle_timeout);
                            session->start(std::bind(&node::priv_msg_handler, self, std::placeholders::_1, std::placeholders::_2),
                                           std::bind(&node::priv_proto_handler, self, std::placeholders::_1, std::placeholders::_2));
                            
                            // send the message requested...
                            session->send_message(msg, false);
                        });
            });
}

void
node::send_message(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::json_message> msg)
{
    this->send_message_str(ep, std::make_shared<bzn::encoded_message>(msg->toStyledString()));
}
