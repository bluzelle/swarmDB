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
#include <http/connection.hpp>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>


namespace
{
    const std::string CREATE_REQ = "create";
    const std::string   READ_REQ = "read";
    const std::string UPDATE_REQ = "update";
    const std::string DELETE_REQ = "delete";

    const std::chrono::seconds HTTP_TIMEOUT{10};

    //             0   1     2      3
    // url format:  /<req>/<uuid>/<key>
    const uint8_t REQUEST_PATH_IDX = 1;
    const uint8_t    UUID_PATH_IDX = 2;
    const uint8_t     KEY_PATH_IDX = 3;
    const size_t     MAX_PATH_SIZE = 4;

    void format_http_response(const boost::beast::string_view& target, const database_response& response, boost::beast::http::response<boost::beast::http::dynamic_body>& http_response)
    {
        if (response.response_case() == database_response::kRedirect)
        {
            // we have a redirect but the leader was unknown? Nothing we can do but return err in that case.
            if (!response.redirect().leader_host().empty())
            {
                http_response.result(boost::beast::http::status::temporary_redirect);
                http_response.set(boost::beast::http::field::location, "http://" +
                    response.redirect().leader_host() + ":" + std::to_string(response.redirect().leader_http_port()) + std::string(target));
            }
            else
            {
                boost::beast::ostream(http_response.body()) << "err";
            }
            return;
        }

        if (response.has_read())
        {
            boost::beast::ostream(http_response.body()) << "ack" << response.read().value();
        }
        else
        {
            boost::beast::ostream(http_response.body()) << ((!response.has_error()) ? "ack" : "err");
        }
    }
}

using namespace bzn::http;


connection::connection(std::shared_ptr<bzn::asio::io_context_base> io_context, std::unique_ptr<bzn::beast::http_socket_base> http_socket, std::shared_ptr<bzn::crud_base> crud)
    : http_socket(std::move(http_socket))
    , deadline_timer(io_context->make_unique_steady_timer())
    , crud(std::move(crud))
{
}


void
connection::start()
{
    std::call_once(this->start_once, &connection::do_read_request, this);
}


void
connection::do_read_request()
{
    this->start_deadline_timer();

    // todo: strands?
    this->http_socket->async_read(this->buffer, this->request,
        [self = shared_from_this()](boost::beast::error_code ec, std::size_t /*bytes_transferred*/)
        {
            if(!ec)
            {
                self->deadline_timer->cancel();

                // extract the path levels from the target...
                std::vector<std::string> path;
                std::string target = self->request.target().to_string();
                boost::split(path, target, boost::is_any_of("/"));

                // test path...
                if (path.size() != MAX_PATH_SIZE)
                {
                    self->response.result(boost::beast::http::status::bad_request);
                }
                else
                {
                    switch (self->request.method())
                    {
                        case boost::beast::http::verb::get:
                        {
                            self->handle_get(path);
                            break;
                        }

                        case boost::beast::http::verb::post:
                        {
                            self->handle_post(path);
                            break;
                        }

                        default:
                        {
                            self->response.result(boost::beast::http::status::bad_request);
                            break;
                        }
                    }
                }

                self->write_response();

                return;
            }

            LOG(error) << "read failed: " << ec.message();
        });
}


void
connection::start_deadline_timer()
{
    this->deadline_timer->expires_from_now(std::chrono::seconds(HTTP_TIMEOUT));

    // todo: strands?
    this->deadline_timer->async_wait(
        [self = shared_from_this()](auto ec)
        {
            if (!ec)
            {
                LOG(info) << "reached deadline timeout -- closing session";
                self->http_socket->close();
                return;
            }
        });
}


void
connection::write_response()
{
    auto self = shared_from_this();

    this->response.version(this->request.version());
    this->response.set(boost::beast::http::field::server, "Bluzelle/" SWARM_VERSION);
    this->response.set(boost::beast::http::field::content_type, "text/plain");
    this->response.set(boost::beast::http::field::content_length, this->response.body().size());
    this->response.keep_alive(false);

    this->http_socket->async_write(
        this->response,
        [self](boost::beast::error_code ec, std::size_t)
        {
            self->http_socket->get_socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        });
}


void
connection::handle_get(const std::vector<std::string>& path)
{
    // Only read is supported by a GET...
    if (path[REQUEST_PATH_IDX] == READ_REQ)
    {
        LOG(debug) << "read: " << path[KEY_PATH_IDX];

        // format request using protobuf...
        database_msg request;
        database_response response;

        request.mutable_header()->set_db_uuid(path[UUID_PATH_IDX]);
        request.mutable_read()->set_key(path[KEY_PATH_IDX]);

        this->crud->handle_read(bzn::message(), request, response);
        format_http_response(this->request.target(), response, this->response);

        return;
    }

    this->response.result(boost::beast::http::status::bad_request);
}


void
connection::handle_post(const std::vector<std::string>& path)
{
    std::stringstream post_data;
    post_data << boost::beast::buffers(this->request.body().data());

    // format request using protobuf...
    database_response response;
    bzn::message crud_msg;
    crud_msg["bzn-api"] = "crud";

    if (path[REQUEST_PATH_IDX] == CREATE_REQ)
    {
        LOG(debug) << "create: " << path[KEY_PATH_IDX] << " : " << post_data.str().substr(0, 1024);

        bzn_msg msg;

        msg.mutable_db()->mutable_header()->set_db_uuid(path[UUID_PATH_IDX]);
        msg.mutable_db()->mutable_create()->set_key(path[KEY_PATH_IDX]);
        msg.mutable_db()->mutable_create()->set_value(post_data.str());

        // todo: temp until we remove all json api or we store the request instead of the json message...
        crud_msg["msg"] = boost::beast::detail::base64_encode(msg.SerializeAsString());

        this->crud->handle_create(crud_msg, msg.db(), response);
        format_http_response(this->request.target(), response, this->response);

        return;
    }
    else if (path[REQUEST_PATH_IDX] == UPDATE_REQ)
    {
        LOG(debug) << "update: " << path[KEY_PATH_IDX] << " : " << post_data.str().substr(0, 1024);

        bzn_msg msg;

        msg.mutable_db()->mutable_header()->set_db_uuid(path[UUID_PATH_IDX]);
        msg.mutable_db()->mutable_update()->set_key(path[KEY_PATH_IDX]);
        msg.mutable_db()->mutable_update()->set_value(post_data.str());

        // todo: temp until we remove all json api or we store the request instead of the json message...
        crud_msg["msg"] = boost::beast::detail::base64_encode(msg.SerializeAsString());

        this->crud->handle_update(crud_msg, msg.db(), response);
        format_http_response(this->request.target(), response, this->response);

        return;
    }
    else if (path[REQUEST_PATH_IDX] == DELETE_REQ)
    {
        LOG(debug) << "delete: " << path[KEY_PATH_IDX] << " : " << post_data.str().substr(0, 1024);

        bzn_msg msg;

        msg.mutable_db()->mutable_header()->set_db_uuid(path[UUID_PATH_IDX]);
        msg.mutable_db()->mutable_delete_()->set_key(path[KEY_PATH_IDX]);

        // todo: temp until we remove all json api or we store the request instead of the json message...
        crud_msg["msg"] = boost::beast::detail::base64_encode(msg.SerializeAsString());

        this->crud->handle_delete(crud_msg, msg.db(), response);
        format_http_response(this->request.target(), response, this->response);

        return;
    }

    this->response.result(boost::beast::http::status::bad_request);
}
