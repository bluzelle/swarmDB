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

    /* todo: add to generate_key?
     *
        openssl dhparam -out dh.pem 2048
        openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem -subj "//C=US\ST=CA\L=Los Angeles\O=Beast\CN=www.example.com"
    */
}


node::node(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::beast::websocket_base> websocket, std::shared_ptr<chaos_base> chaos,
    const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::crypto_base> crypto, std::shared_ptr<bzn::options_base> options, std::shared_ptr<bzn::monitor_base> monitor)
    : tcp_acceptor(io_context->make_unique_tcp_acceptor(ep))
    , io_context(std::move(io_context))
    , websocket(std::move(websocket))
    , chaos(std::move(chaos))
    , crypto(std::move(crypto))
    , options(std::move(options))
    , monitor(std::move(monitor))
{
    this->initialize_ssl_contexts();
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
    std::lock_guard<std::shared_mutex> lock(this->message_map_mutex);

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
node::register_error_handler(std::function<void(const boost::asio::ip::tcp::endpoint& ep
    , const boost::system::error_code&)> error_handler)
{
    this->error_callback = std::move(error_handler);
}

void
node::do_accept()
{
    std::shared_ptr<bzn::asio::strand_base> strand = this->io_context->make_unique_strand();
    this->acceptor_socket = this->io_context->make_unique_tcp_socket(*strand);

    this->tcp_acceptor->async_accept(*this->acceptor_socket,
        [self = shared_from_this(), strand](const boost::system::error_code& ec)
        {
            if (ec)
            {
                LOG(error) << "accept failed: " << ec.message();
            }
            else
            {
                auto ep = self->acceptor_socket->remote_endpoint();
                auto key = self->key_from_ep(ep);

                // set tcp_nodelay option...
                boost::system::error_code option_ec;
                self->acceptor_socket->get_tcp_socket().set_option(boost::asio::ip::tcp::no_delay(true), option_ec);
                if (option_ec)
                {
                    LOG(warning) << "failed to set socket option TCP_NODELAY: " << option_ec.message();
                }
#ifndef __APPLE__
                int flags = 1;
                if (setsockopt(self->acceptor_socket->get_tcp_socket().native_handle(), SOL_TCP, TCP_QUICKACK, &flags, sizeof(flags)))
                {
                    LOG(warning) << "failed to set socket option TCP_QUICKACK: " << errno;
                }
#endif
                std::shared_ptr<bzn::beast::websocket_stream_base> ws =
                    (!self->options->get_wss_enabled()) ? self->websocket->make_websocket_stream(self->acceptor_socket->get_tcp_socket()) :
                    self->websocket->make_websocket_secure_stream(self->acceptor_socket->get_tcp_socket(), *self->server_ctx);

                auto session = std::make_shared<bzn::session>(
                        self->io_context
                        , ++self->session_id_counter
                        , ep
                        , self->chaos
                        , std::bind(&node::priv_protobuf_handler, self, std::placeholders::_1, std::placeholders::_2)
                        , self->options->get_ws_idle_timeout()
                        , std::list<bzn::session_shutdown_handler>{[](){}}
                        , self->crypto
                        , self->monitor
                        , self->options
                        , strand);

                session->accept(std::move(ws));

                LOG(info) << "accepting new incoming connection with " << key;
                // Do not attempt to identify the incoming session; one ip address could be running multiple daemons
                // and we can't identify them based on the outgoing ports they choose
            }

            self->do_accept();
        });
}


void
node::priv_protobuf_handler(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session)
{
    std::shared_lock<std::shared_mutex> lock(this->message_map_mutex); // lock for read access

    if (msg.swarm_id() != this->options->get_swarm_id())
    {
        LOG(error) << "Dropping message with invalid swarm id: " << msg.ShortDebugString().substr(0, MAX_MESSAGE_SIZE);
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


std::shared_ptr<bzn::session_base>
node::find_session(const boost::asio::ip::tcp::endpoint& ep)
{
    std::shared_ptr<bzn::session_base> session;
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
                , std::list<bzn::session_shutdown_handler>{std::bind(&node::priv_session_shutdown_handler, shared_from_this(), key)}
                , this->crypto
                , this->monitor
                , this->options
                , std::nullopt
                , this->client_ctx);
        session->open(this->websocket, [self = shared_from_this(), ep](const boost::system::error_code& ec)
        {
            if (ec && self->error_callback)
            {
                self->error_callback(ep, ec);
            }
        });
        this->sessions.insert_or_assign(key, session);
    }
    return session;
}


void
node::send_message_str(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::encoded_message> msg)
{
    this->find_session(ep)->send_message(msg);
}


void
node::send_signed_message(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn_envelope> msg)
{
    this->find_session(ep)->send_signed_message(msg);
}


void
node::multicast_signed_message(std::shared_ptr<std::vector<boost::asio::ip::tcp::endpoint>> eps, std::shared_ptr<bzn_envelope> msg)
{
    this->io_context->post(
            [self =  shared_from_this(), msg, eps]()
            {
                if (msg->signature().empty())
                {
                    self->crypto->sign(*msg);
                }

                for (const auto& ep : *eps)
                {
                    self->send_signed_message(ep, msg);
                }
            });
}


ep_key_t
node::key_from_ep(const boost::asio::ip::tcp::endpoint& ep)
{
    return ep.address().to_string() + ":" + std::to_string(ep.port());
}


void
node::send_signed_message(const bzn::uuid_t& uuid, std::shared_ptr<bzn_envelope> msg)
{
    try
    {
        auto point_of_contact_address = this->pbft->get_peer_by_uuid(uuid);
        boost::asio::ip::tcp::endpoint endpoint{bzn::make_endpoint(point_of_contact_address)};
        this->send_signed_message(endpoint, msg);
    }
    catch (const std::runtime_error& err)
    {
        LOG(error) << "Unable to send message to " << uuid << ": " << err.what();
    }
}

void
node::send_maybe_signed_message(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn_envelope> msg)
{
    if (this->options->get_peer_message_signing())
    {
        this->find_session(ep)->send_signed_message(msg);
    }
    else
    {
        msg->set_sender(this->options->get_uuid());
        msg->set_swarm_id(this->options->get_swarm_id());
        this->find_session(ep)->send_message(std::make_shared<std::string>(msg->SerializeAsString()));
    }
}

void
node::multicast_maybe_signed_message(std::shared_ptr<std::vector<boost::asio::ip::tcp::endpoint>> eps, std::shared_ptr<bzn_envelope> msg)
{
    this->io_context->post(
        [self =  shared_from_this(), msg, eps]()
        {
            for (const auto& ep : *eps)
            {
                self->send_maybe_signed_message(ep, msg);
            }
        });
}

void
node::send_maybe_signed_message(const bzn::uuid_t& uuid, std::shared_ptr<bzn_envelope> msg)
{
    try
    {
        auto point_of_contact_address = this->pbft->get_peer_by_uuid(uuid);
        boost::asio::ip::tcp::endpoint endpoint{bzn::make_endpoint(point_of_contact_address)};
        this->send_maybe_signed_message(endpoint, msg);
    }
    catch (const std::runtime_error& err)
    {
        LOG(error) << "Unable to send message to " << uuid << ": " << err.what();
    }
}



void
node::initialize_ssl_contexts()
{
    if (!this->options->get_wss_enabled())
    {
        // nothing to do...
        LOG(info) << "WSS disabled";
        return;
    }

    LOG(info) << "WSS enabled";

    this->server_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_server);

    // server (inbound)
    this->server_ctx->set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::single_dh_use);

    this->server_ctx->use_certificate_chain_file(this->options->get_wss_server_certificate_file());

    this->server_ctx->use_private_key_file(this->options->get_wss_server_private_key_file(),
        boost::asio::ssl::context::file_format::pem);

    if (!this->options->get_wss_server_dh_params_file().empty())
    {
        this->server_ctx->use_tmp_dh_file(this->options->get_wss_server_dh_params_file());
    }

    // client (outbound)
    this->client_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);

    // set default paths for finding CA certificates...
    //this->client_ctx.set_default_verify_paths();
}
