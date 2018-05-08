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

using namespace bzn;


session::session(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::beast::websocket_stream_base> websocket)
    : strand(io_context->make_unique_strand())
    , io_context(std::move(io_context))
    , websocket(std::move(websocket))
{
}


session::~session()
{
    if (this->websocket->is_open())
    {
        boost::system::error_code ec;

        this->websocket->get_websocket().close(boost::beast::websocket::close_code::normal, ec);

        if (ec)
        {
           LOG(debug) << "failed to close websocket: " << ec.message();
        }
    }
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
                self->do_read(nullptr);
            }
        );
    }
}


void
session::do_read(bzn::message_handler reply_handler)
{
    this->buffer.consume(this->buffer.size());

    this->websocket->async_read(this->buffer,
        [self = shared_from_this(), reply_handler](auto ec, auto /*bytes_transferred*/)
        {
            if (ec)
            {
                LOG(error) << "websocket read failed: " << ec.message();
                return;
            }

            // get the message...
            std::stringstream ss;
            ss << boost::beast::buffers(self->buffer.data());

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

            // handler may schedule a write asking for the response, if not then we are shutting down...
            if (reply_handler)
            {
                reply_handler(msg, self);
            }
            else
            {
                self->handler(msg, self);
            }
        });
}


// todo: We will want to pipeline other messages on this stream, so we will not
// use the reply_handler, but instead rely on the node to continue parsing messages
// and dispatching them accordingly. All uses of send_message do not use the reply_handler anyway.
void
session::send_message(const bzn::message& msg, bzn::message_handler reply_handler)
{
    // todo: we will want to use a shared_ptr!
    this->send_msg = msg.toStyledString();

    this->websocket->async_write(
        boost::asio::buffer(this->send_msg),
        this->strand->wrap(
            [self = shared_from_this(), reply_handler](auto ec, auto bytes_transferred)
            {
                if (ec)
                {
                    LOG(error) << "websocket write failed: " << ec.message() << " bytes: " << bytes_transferred;

                    if(reply_handler)
                    {
                        reply_handler(bzn::message(), nullptr);
                    }
                    return;
                }

                if (reply_handler)
                {
                    LOG(debug) << "response requested";

                    self->do_read(reply_handler);

                    return;
                }

                self->close();
            }));
}


void
session::close()
{
    this->websocket->async_close(boost::beast::websocket::close_code::normal,
        [self = shared_from_this()](auto ec)
        {
            if (ec)
            {
                LOG(error) << "failed to close websocket: " << ec.message();
            }
        });
}
