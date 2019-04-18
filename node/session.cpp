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

#include <node/session.hpp>
#include <node/node.hpp>
#include <sstream>
#include <boost/beast/websocket/error.hpp>

using namespace bzn;

session::session(
        std::shared_ptr<bzn::asio::io_context_base> io_context,
        bzn::session_id session_id,
        boost::asio::ip::tcp::endpoint ep,
        std::shared_ptr<bzn::chaos_base> chaos,
        bzn::protobuf_handler proto_handler,
        std::chrono::milliseconds ws_idle_timeout,
        std::list<bzn::session_shutdown_handler> shutdown_handlers,
        std::shared_ptr<bzn::crypto_base> crypto,
        std::shared_ptr<bzn::monitor_base> monitor,
        std::shared_ptr<bzn::options_base> options
)
        : session_id(session_id)
        , ep(std::move(ep))
        , io_context(std::move(io_context))
        , chaos(std::move(chaos))
        , proto_handler(std::move(proto_handler))
        , shutdown_handlers(std::move(shutdown_handlers))
        , idle_timer(this->io_context->make_unique_steady_timer())
        , ws_idle_timeout(std::move(ws_idle_timeout))
        , strand(this->io_context->make_unique_strand())
        , write_buffer(nullptr, 0)
        , crypto(std::move(crypto))
        , monitor(std::move(monitor))
        , options(std::move(options))
{
    LOG(debug) << "creating session " << std::to_string(session_id);
}

void
session::start_idle_timeout()
{
    this->activity = false;

    this->idle_timer->expires_from_now(this->ws_idle_timeout);
    this->idle_timer->async_wait(
        this->strand->wrap([self = shared_from_this()](auto /*ec*/)
        {
            if (!self->activity)
            {
                LOG(info) << "Closing session " << std::to_string(self->session_id) << " due to inactivity";
                self->private_close();
                return;
            }

            if (!self->closing)
            {
                self->start_idle_timeout();
            }
        }));
}

void
session::open(std::shared_ptr<bzn::beast::websocket_base> ws_factory)
{
    this->io_context->post(this->strand->wrap([self = shared_from_this(), ws_factory]()
    {

        std::shared_ptr<bzn::asio::tcp_socket_base> socket = self->io_context->make_unique_tcp_socket();
        socket->async_connect(self->ep,
            self->strand->wrap([self, socket, ws_factory](const boost::system::error_code& ec)
            {
                self->activity = true;

                if(ec)
                {
                    LOG(error) << "failed to connect to: " << self->ep.address().to_string() << ":" << self->ep.port() << " - " << ec.message();
                    return;
                }

                // we've completed the handshake...
                
                // set tcp_nodelay option
                boost::system::error_code option_ec;
                socket->get_tcp_socket().set_option(boost::asio::ip::tcp::no_delay(true), option_ec);
                if (option_ec)
                {
                    LOG(warning) << "failed to set socket option TCP_NODELAY: " << option_ec.message();
                }
#ifndef __APPLE__
                int flags = 1;
                if (setsockopt(socket->get_tcp_socket().native_handle(), SOL_TCP, TCP_QUICKACK, &flags, sizeof(flags)))
                {
                    LOG(warning) << "failed to set socket option TCP_QUICKACK: " << errno;
                }
#endif
                self->websocket = ws_factory->make_unique_websocket_stream(socket->get_tcp_socket());
                self->websocket->async_handshake(self->ep.address().to_string(), "/",
                    self->strand->wrap([self, ws_factory](const boost::system::error_code& ec)
                    {
                        self->activity = true;

                        if (ec)
                        {
                            LOG(error) << "handshake failed: " << ec.message();

                            return;
                        }

                        self->start_idle_timeout();
                        self->do_read();
                        self->do_write();
                    }));
            }));
    }));
}

void
session::accept(std::shared_ptr<bzn::beast::websocket_stream_base> ws)
{
    this->io_context->post(this->strand->wrap([self = shared_from_this(), ws]()
    {

        self->websocket = std::move(ws);
        self->websocket->async_accept(
                self->strand->wrap(
                        [self](boost::system::error_code ec)
                        {
                            self->activity = true;

                            if (ec)
                            {
                                LOG(error) << "websocket accept failed: " << ec.message();
                                return;
                            }

                            self->monitor->send_counter(statistic::session_opened);
                            self->start_idle_timeout();
                            self->do_read();
                            self->do_write();
                        }
                )
       );
   }));
}

void
session::add_shutdown_handler(const bzn::session_shutdown_handler handler)
{
    this->io_context->post(this->strand->wrap([handler, self = shared_from_this()]()
    {
        self->shutdown_handlers.push_back(handler);
    }));
}

void
session::do_read()
{
    // assume we are invoked inside the strand

    auto buffer = std::make_shared<boost::beast::multi_buffer>();

    if (this->reading || !this->is_open() || this->closing)
    {
        return;
    }

    this->reading = true;

    this->websocket->async_read(*buffer,
        this->strand->wrap([self = shared_from_this(), buffer](boost::system::error_code ec, auto /*bytes_transferred*/)
        {
            self->activity = true;

            if(ec)
            {
                // don't log close of websocket...
                if (ec != boost::beast::websocket::error::closed && ec != boost::asio::error::eof)
                {
                    LOG(error) << "websocket read failed: " << ec.message();
                }
                if (ec != boost::beast::websocket::error::closed)
                {
                    self->private_close();
                }
                return;
            }

            self->io_context->post(
                    [self, buffer]()
                    {
                        // get the message...
                        std::stringstream ss;
                        ss << boost::beast::buffers(buffer->data());

                        bzn_envelope proto_msg;

                        if (proto_msg.ParseFromIstream(&ss))
                        {
                            self->proto_handler(proto_msg, self);
                        }
                        else
                        {
                            LOG(error) << "Failed to parse incoming message";
                        }
                    });

            self->reading = false;
            self->do_read();

        })
    );
}

void
session::do_write()
{
    // assume we are invoked inside the strand

    if(this->writing || !this->is_open() || this->write_queue.empty() || this->closing)
    {
        return;
    }

    this->writing = true;

    auto msg = this->write_queue.front();
    this->write_queue.pop_front();

    this->websocket->binary(true);
    this->write_buffer = boost::asio::buffer(*msg);
    this->websocket->async_write(this->write_buffer,
        this->strand->wrap([self = shared_from_this(), msg](boost::system::error_code ec, auto bytes_transferred)
        {
            self->activity = true;

            if (!ec)
            {
                self->monitor->send_counter(statistic::message_sent);
            }
            self->monitor->send_counter(statistic::message_sent_bytes, bytes_transferred);

            if(ec)
            {
                // don't log close of websocket...
                if (ec != boost::beast::websocket::error::closed && ec != boost::asio::error::eof)
                {
                    LOG(error) << "websocket read failed: " << ec.message();
                }

                self->write_queue.push_front(msg);
                if (ec != boost::beast::websocket::error::closed)
                {
                    self->private_close();
                }
                return;
            }

            self->writing = false;
            self->do_write();
        }));
}

void
session::send_signed_message(std::shared_ptr<bzn_envelope> msg)
{
    this->io_context->post(
            [self = shared_from_this(), msg]()
            {
                msg->set_swarm_id(self->options->get_swarm_id());
                if (msg->signature().empty())
                {
                    self->crypto->sign(*msg);
                }

                self->send_message(std::make_shared<std::string>(msg->SerializeAsString()));
            });
}

void
session::send_message(std::shared_ptr<bzn::encoded_message> msg)
{
    if (this->chaos->is_message_delayed())
    {
        LOG(debug) << "chaos testing delaying message";
        this->chaos->reschedule_message(std::bind(static_cast<void(session::*)(std::shared_ptr<std::string>l)>(&session::send_message), shared_from_this(), std::move(msg)));
        return;
    }

    if (this->chaos->is_message_dropped())
    {
        LOG(debug) << "chaos testing dropping message";
        return;
    }

    this->io_context->post(this->strand->wrap([self = shared_from_this(), msg]()
    {
        self->write_queue.push_back(msg);
        self->do_write();
    }));
}

void
session::close()
{
    this->io_context->post(this->strand->wrap([self = shared_from_this()](){self->private_close();}));
}

void
session::private_close()
{
    // assume we are invoked inside the strand

    // TODO: re-open socket later if we still have messages to send? (KEP-1037)
    if (this->closing)
    {
        return;
    }

    this->closing = true;
    LOG(debug) << "closing session " << std::to_string(this->session_id);

    for(const auto& handler : this->shutdown_handlers)
    {
        this->io_context->post(handler);
    }

    if (this->websocket && this->websocket->is_open())
    {
        this->websocket->async_close(boost::beast::websocket::close_code::normal,
            this->strand->wrap([self = shared_from_this()](auto ec)
            {
                if (ec)
                {
                    LOG(error) << "failed to close websocket: " << ec.message();
                }
            }));
    }

    this->idle_timer->cancel();
}

bool
session::is_open() const
{
    return this->websocket && this->websocket->is_open() && !this->closing;
}

session::~session()
{
    if (!this->write_queue.empty())
    {
        LOG(warning) << "dropping session with " << this->write_queue.size() << " messages left in its write queue";
    }
}

