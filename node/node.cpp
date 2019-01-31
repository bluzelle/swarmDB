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
#include <utils/make_endpoint.hpp>
#include <pbft/pbft.hpp>

using namespace bzn;

namespace
{
    const std::string BZN_API_KEY = "bzn-api";
}


node::node(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::beast::websocket_base> websocket, std::shared_ptr<chaos_base> chaos,
    const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::crypto_base> crypto, std::shared_ptr<bzn::options_base> options)
    : tcp_acceptor(io_context->make_unique_tcp_acceptor(ep))
    , io_context(std::move(io_context))
    , websocket(std::move(websocket))
    , chaos(std::move(chaos))
    , crypto(std::move(crypto))
    , options(std::move(options))
{
}

void
node::start(std::shared_ptr<bzn::pbft_base> pbft)
{
    std::call_once(this->start_once, [&]
        {
            this->pbft = std::move(pbft);
            this->do_accept();
        });
}

bool
node::register_for_message(const bzn_envelope::PayloadCase type, bzn::protobuf_handler msg_handler)
{
    std::lock_guard<std::mutex> lock(this->message_map_mutex);

    // never allow!
    if (!msg_handler)
    {
        return false;
    }

    if (this->protobuf_map.find(type) != this->protobuf_map.end())
    {
        LOG(debug) << type << " message type already registered";

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
                auto key = self->key_from_ep(ep);

                std::shared_ptr<bzn::beast::websocket_stream_base> ws = self->websocket->make_unique_websocket_stream(
                    self->acceptor_socket->get_tcp_socket());

                auto session = std::make_shared<bzn::session>(
                        self->io_context
                        , ++self->session_id_counter
                        , ep
                        , self->chaos
                        , std::bind(&node::priv_protobuf_handler, self, std::placeholders::_1, std::placeholders::_2)
                        , self->options->get_ws_idle_timeout()
                        , [](){});

                session->accept(std::move(ws));

                LOG(info) << "accepting new incomming connection with " << key;
                // Do not attempt to identify the incoming session; one ip address could be running multiple daemons
                // and we can't identify them based on the outgoing ports they choose
            }

            self->do_accept();
        });
}

void
node::priv_protobuf_handler(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session)
{
    std::lock_guard<std::mutex> lock(this->message_map_mutex);

    if ((!msg.sender().empty()) && (!this->crypto->verify(msg)))
    {
        LOG(error) << "Dropping message with invalid signature: " << msg.ShortDebugString().substr(0, MAX_MESSAGE_SIZE);
        return;
    }

    if (auto it = this->protobuf_map.find(msg.payload_case()); it != this->protobuf_map.end())
    {
        it->second(msg, std::move(session));
    }
    else
    {
        LOG(debug) << "no handler for message type " << msg.payload_case();
    }

}

void
node::priv_session_shutdown_handler(const ep_key_t& ep_key)
{
    std::shared_ptr<bzn::session_base> session;
    std::lock_guard<std::mutex> lock(this->session_map_mutex);
    if (this->sessions.find(ep_key) != this->sessions.end() && (session = this->sessions.at(ep_key).lock()) && session->is_open())
    {
        // the session may have already been replaced, and we don't want to remove the new one if so
        return;
    }
    this->sessions.erase(ep_key);
}

void
node::send_message_str(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::encoded_message> msg) {
    std::shared_ptr<bzn::session_base> session;

    {
        std::lock_guard<std::mutex> lock(this->session_map_mutex);
        auto key = this->key_from_ep(ep);

        if (this->sessions.find(key) == this->sessions.end() || !(session = this->sessions.at(key).lock()) || !session->is_open())
        {
            session = std::make_shared<bzn::session>(
                    this->io_context
                    , ++this->session_id_counter
                    , ep
                    , this->chaos
                    , std::bind(&node::priv_protobuf_handler, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
                    , this->options->get_ws_idle_timeout()
                    , std::bind(&node::priv_session_shutdown_handler, shared_from_this(), key));
            session->open(this->websocket);
            sessions.insert_or_assign(key, session);
        }
        // else session was assigned by the condition
    }

    session->send_message(msg);
}

void
node::send_message(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn_envelope> msg)
{
    if (msg->sender().empty())
    {
        msg->set_sender(this->options->get_uuid());
    }

    if (msg->signature().empty())
    {
        this->crypto->sign(*msg);
    }

    this->send_message_str(ep, std::make_shared<std::string>(msg->SerializeAsString()));
}

ep_key_t
node::key_from_ep(const boost::asio::ip::tcp::endpoint& ep)
{
    return ep.address().to_string() + ":" + std::to_string(ep.port());
}

void
node::send_message(const bzn::uuid_t &uuid, std::shared_ptr<bzn_envelope> msg)
{
    try
    {
        auto point_of_contact_address = this->pbft->get_peer_by_uuid(uuid);
        boost::asio::ip::tcp::endpoint endpoint{bzn::make_endpoint(point_of_contact_address)};
        this->send_message(endpoint, msg);
    }
    catch (const std::runtime_error& err)
    {
        LOG(error) << "Unable to send message to " << uuid << ": " << err.what();
    }
}
