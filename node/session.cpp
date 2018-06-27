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
#include <sstream>

namespace
{
    const std::chrono::seconds DEFAULT_WS_TIMEOUT_MS{10};
}


using namespace bzn;


session::session(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::beast::websocket_stream_base> websocket, const std::chrono::milliseconds& ws_idle_timeout)
    : strand(io_context->make_unique_strand())
    , websocket(std::move(websocket))
    , idle_timer(io_context->make_unique_steady_timer())
    , ws_idle_timeout(ws_idle_timeout.count() ? ws_idle_timeout : DEFAULT_WS_TIMEOUT_MS)
{
}


void
session::start(bzn::message_handler handler)
{
    this->handler = std::move(handler);

    // If we haven't completed a handshake then we are accepting one...
    if (!this->websocket->is_open())
    {
        this->websocket->async_accept(
            [self = shared_from_this()](boost::system::error_code ec)
            {
                if (ec)
                {
                    LOG(error) << "websocket accept failed: " << ec.message();
                    return;
                }

                // schedule read...
                self->do_read();
            }
        );
    }
}


void
session::do_read()
{
    auto buffer = std::make_shared<boost::beast::multi_buffer>();

    this->start_idle_timeout();

    this->websocket->async_read(*buffer,
        this->strand->wrap(
        [self = shared_from_this(), buffer](boost::system::error_code ec, auto /*bytes_transferred*/)
        {
            self->idle_timer->cancel();

            if (ec)
            {
                LOG(error) << "websocket read failed: " << ec.message();
                return;
            }

            // get the message...
            std::stringstream ss;
            ss << boost::beast::buffers(buffer->data());

            Json::Value msg;
            Json::Reader reader;

            if (!reader.parse(ss.str(), msg))
            {
                LOG(error) << "Failed to parse: " << reader.getFormattedErrorMessages();

                // Only a unit test should change this flag since we can't intercept the buffer and modify it.
                if (!self->ignore_json_errors)
                {
                    self->close();
                    return;
                }
            }

            // call subscriber...
            self->handler(msg, self);
        }));
}


void
session::send_message(std::shared_ptr<bzn::message> msg, const bool end_session)
{
    this->send_message(std::make_shared<std::string>(msg->toStyledString()), end_session);
}


void
session::send_message(std::shared_ptr<std::string> msg, const bool end_session)
{
    this->idle_timer->cancel(); // kill timer for duration of write...

    this->websocket->get_websocket().binary(true);

    this->websocket->async_write(
        boost::asio::buffer(*msg),
        this->strand->wrap(
            [self = shared_from_this(), msg, end_session](auto ec, auto bytes_transferred)
            {
                if (ec)
                {
                    LOG(error) << "websocket write failed: " << ec.message() << " bytes: " << bytes_transferred;

                    self->close();
                    return;
                }

                if (end_session)
                {
                    self->close();
                    return;
                }

                self->do_read();
            }));
}


void
session::close()
{
    this->idle_timer->cancel();

    if (this->websocket->is_open())
    {
        LOG(info) << "closing session";

        this->websocket->async_close(boost::beast::websocket::close_code::normal,
            [self = shared_from_this()](auto ec)
            {
                if (ec)
                {
                    LOG(error) << "failed to close websocket: " << ec.message();
                }
            });
    }
}


void
session::start_idle_timeout()
{
    this->idle_timer->cancel();

    LOG(debug) << "resetting " << this->ws_idle_timeout.count() << "ms idle timer";

    this->idle_timer->expires_from_now(this->ws_idle_timeout);

    this->idle_timer->async_wait(
        [self = shared_from_this()](auto ec)
        {
            if (!ec)
            {
                LOG(info) << "reached idle timeout -- closing session: " << ec.message();

                self->websocket->async_close(boost::beast::websocket::close_code::normal,
                    [self](auto ec)
                    {
                        if (ec)
                        {
                            LOG(error) << "failed to close websocket: " << ec.message();
                        }
                    });

                return;
            }
        });
}
