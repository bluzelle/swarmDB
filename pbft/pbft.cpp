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

#include <pbft/pbft.hpp>
#include <utils/make_endpoint.hpp>
#include <google/protobuf/text_format.h>
#include <cstdint>
#include <memory>
#include <boost/beast/core/detail/base64.hpp>
#include <algorithm>
#include <numeric>
#include <crud/crud_base.hpp>

namespace
{
    const std::chrono::milliseconds heartbeat_interval{std::chrono::milliseconds(5000)};
}

using namespace bzn;

pbft::pbft(
        std::shared_ptr<bzn::node_base> node
        , std::shared_ptr<bzn::asio::io_context_base> io_context
        , const bzn::peers_list_t& peers
        , bzn::uuid_t uuid
        , std::shared_ptr<pbft_service_base> service
        , std::shared_ptr<pbft_failure_detector_base> failure_detector
)
        : node(std::move(node))
        , uuid(std::move(uuid))
        , service(std::move(service))
        , failure_detector(std::move(failure_detector))
        , io_context(io_context)
        , audit_heartbeat_timer(this->io_context->make_unique_steady_timer())
{
    if (peers.empty())
    {
        throw std::runtime_error("No peers found!");
    }

    // We cannot directly sort the peers list or a copy of it because peer addresses have const members
    std::vector<peer_address_t> unordered_peers_list;
    std::copy(peers.begin(), peers.end(), std::back_inserter(unordered_peers_list));

    std::vector<size_t> indicies(peers.size());
    std::iota(indicies.begin(), indicies.end(), 0);

    std::sort(indicies.begin(), indicies.end(),
            [&unordered_peers_list](const auto& i1, const auto& i2)
            {
                return unordered_peers_list[i1].uuid < unordered_peers_list[i2].uuid;
            }
    );

    std::transform(indicies.begin(), indicies.end(), std::back_inserter(this->peer_index),
            [&unordered_peers_list](auto& peer_index)
            {
                return unordered_peers_list[peer_index];
            }
    );
}

void
pbft::start()
{
    std::call_once(this->start_once,
            [this]()
            {
                this->node->register_for_message("pbft",
                        std::bind(&pbft::handle_pbft_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                this->node->register_for_message("database",
                        std::bind(&pbft::handle_database_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                this->audit_heartbeat_timer->expires_from_now(heartbeat_interval);
                this->audit_heartbeat_timer->async_wait(
                        std::bind(&pbft::handle_audit_heartbeat_timeout, shared_from_this(), std::placeholders::_1));

                this->failure_detector->register_failure_handler(
                        [weak_this = this->weak_from_this()]()
                        {
                            auto strong_this = weak_this.lock();
                            if(strong_this)
                            {
                                strong_this->handle_failure();
                            }
                        }
                        );
            });
}

void
pbft::handle_audit_heartbeat_timeout(const boost::system::error_code& ec)
{
    if (ec)
    {
        LOG(error) << "pbft audit heartbeat canceled? " << ec.message();
        return;
    }

    if (this->is_primary() && this->audit_enabled)
    {
        audit_message msg;
        msg.mutable_primary_status()->set_view(this->view);
        msg.mutable_primary_status()->set_primary(this->uuid);

        this->broadcast(this->wrap_message(msg));

    }

    this->audit_heartbeat_timer->expires_from_now(heartbeat_interval);
    this->audit_heartbeat_timer->async_wait(std::bind(&pbft::handle_audit_heartbeat_timeout, shared_from_this(), std::placeholders::_1));
}

void
pbft::handle_pbft_message(const bzn::message& json, std::shared_ptr<bzn::session_base> /*session*/)
{
    pbft_msg msg;
    if (!msg.ParseFromString(boost::beast::detail::base64_decode(json["pbft-data"].asString())))
    {
        LOG(error) << "Got invalid message " << json.toStyledString().substr(0, MAX_MESSAGE_SIZE);
        return;
    }
    this->handle_message(msg);
}

void
pbft::handle_message(const pbft_msg& msg)
{

    LOG(debug) << "Received message: " << msg.ShortDebugString().substr(0, MAX_MESSAGE_SIZE);

    if (!this->preliminary_filter_msg(msg))
    {
        return;
    }

    if(msg.has_request())
    {
        this->failure_detector->request_seen(msg.request());
    }

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    switch (msg.type())
    {
        case PBFT_MSG_REQUEST :
            this->handle_request(msg.request());
            break;
        case PBFT_MSG_PREPREPARE :
            this->handle_preprepare(msg);
            break;
        case PBFT_MSG_PREPARE :
            this->handle_prepare(msg);
            break;
        case PBFT_MSG_COMMIT :
            this->handle_commit(msg);
            break;
        default :
            throw std::runtime_error("Unsupported message type");
    }
}

bool
pbft::preliminary_filter_msg(const pbft_msg& msg)
{

    // TODO: Crypto verification goes here - KEP-331, KEP-345

    auto t = msg.type();
    if (t == PBFT_MSG_PREPREPARE || t == PBFT_MSG_PREPARE || t == PBFT_MSG_COMMIT)
    {
        if (msg.view() != this->view)
        {
            LOG(debug) << "Dropping message because it has the wrong view number";
            return false;
        }

        if (msg.sequence() < this->low_water_mark)
        {
            LOG(debug) << "Dropping message becasue it has an unreasonable sequence number";
            return false;
        }

        if (msg.sequence() > this->high_water_mark)
        {
            LOG(debug) << "Dropping message becasue it has an unreasonable sequence number";
            return false;
        }
    }

    return true;
}

void
pbft::handle_request(const pbft_request& msg, std::shared_ptr<bzn::session_base> session)
{
    if (!this->is_primary())
    {
        LOG(error) << "Ignoring client request because I am not the primary";
        // TODO - KEP-327
        return;
    }

    //TODO: conditionally discard based on timestamp - KEP-328

    //TODO: keep track of what requests we've seen based on timestamp and only send preprepares once - KEP-329

    const uint64_t request_seq = this->next_issued_sequence_number++;
    auto op = this->find_operation(this->view, request_seq, msg);

    if(session)
    {
        op->set_session(std::move(session));
    }

    this->do_preprepare(std::move(op));
}

void
pbft::handle_preprepare(const pbft_msg& msg)
{

    // If we've already accepted a preprepare for this view+sequence, and it's not this one, then we should reject this one
    // Note that if we get the same preprepare more than once, we can still accept it
    const log_key_t log_key(msg.view(), msg.sequence());

    if (auto lookup = this->accepted_preprepares.find(log_key);
        lookup != this->accepted_preprepares.end()
        && std::get<2>(lookup->second) != pbft_operation::request_hash(msg.request()))
    {

        LOG(debug) << "Rejecting preprepare because I've already accepted a conflicting one \n";
        return;
    }
    else
    {
        auto op = this->find_operation(msg);
        op->record_preprepare();

        // This assignment will be redundant if we've seen this preprepare before, but that's fine
        accepted_preprepares[log_key] = op->get_operation_key();

        op = this->do_preprepared(std::move(op));
        this->maybe_advance_operation_state(std::move(op));
    }
}

void
pbft::handle_prepare(const pbft_msg& msg)
{

    // Prepare messages are never rejected, assuming the sanity checks passed
    auto op = this->find_operation(msg);

    op->record_prepare(msg);
    this->maybe_advance_operation_state(std::move(op));
}

void
pbft::handle_commit(const pbft_msg& msg)
{

    // Commit messages are never rejected, assuming  the sanity checks passed
    auto op = this->find_operation(msg);

    op->record_commit(msg);
    this->maybe_advance_operation_state(std::move(op));
}

void
pbft::broadcast(const bzn::message& json)
{
    auto json_ptr = std::make_shared<bzn::message>(json);

    LOG(debug) << "Broadcasting message " + json.toStyledString();

    for (const auto& peer : this->peer_index)
    {
        this->node->send_message(make_endpoint(peer), json_ptr);
    }
}

void
pbft::maybe_advance_operation_state(std::shared_ptr<pbft_operation> op)
{
    if (op->get_state() == pbft_operation_state::prepare && op->is_prepared())
    {
        op = this->do_prepared(std::move(op));
    }

    if (op->get_state() == pbft_operation_state::commit && op->is_committed())
    {
        this->do_committed(std::move(op));
    }
}

pbft_msg
pbft::common_message_setup(const std::shared_ptr<pbft_operation>& op, pbft_msg_type type)
{
    pbft_msg msg;
    msg.set_view(op->view);
    msg.set_sequence(op->sequence);
    msg.set_allocated_request(new pbft_request(op->request));
    msg.set_type(type);


    // Some message types don't need this, but it's cleaner to always include it
    msg.set_sender(this->uuid);

    return msg;
}

void
pbft::do_preprepare(std::shared_ptr<pbft_operation> op)
{
    LOG(debug) << "Doing preprepare for operation " << op->debug_string();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPREPARE);

    this->broadcast(this->wrap_message(msg, "preprepare"));
}

std::shared_ptr<pbft_operation>
pbft::do_preprepared(std::shared_ptr<pbft_operation> op)
{
    LOG(debug) << "Entering prepare phase for operation " << op->debug_string();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPARE);

    this->broadcast(this->wrap_message(msg, "prepare"));

    return std::move(op);
}

std::shared_ptr<pbft_operation>
pbft::do_prepared(std::shared_ptr<pbft_operation> op)
{
    LOG(debug) << "Entering commit phase for operation " << op->debug_string();
    op->begin_commit_phase();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_COMMIT);

    this->broadcast(this->wrap_message(msg, "commit"));

    return std::move(op);
}

void
pbft::do_committed(std::shared_ptr<pbft_operation> op)
{
    LOG(debug) << "Operation " << op->debug_string() << " is committed-local";
    op->end_commit_phase();

    if (this->audit_enabled)
    {
        audit_message msg;
        msg.mutable_pbft_commit()->set_operation(pbft_operation::request_hash(op->request));
        msg.mutable_pbft_commit()->set_sequence_number(op->sequence);
        msg.mutable_pbft_commit()->set_sender_uuid(this->uuid);

        this->broadcast(this->wrap_message(msg));
    }

    this->service->apply_operation(std::move(op));
}

size_t
pbft::outstanding_operations_count() const
{
    return operations.size();
}

bool
pbft::is_primary() const
{
    return this->get_primary().uuid == this->uuid;
}

const peer_address_t&
pbft::get_primary() const
{
    return this->peer_index[this->view % this->peer_index.size()];
}

// Find this node's record of an operation (creating a new record for it if this is the first time we've heard of it)
std::shared_ptr<pbft_operation>
pbft::find_operation(const pbft_msg& msg)
{
    return this->find_operation(msg.view(), msg.sequence(), msg.request());
}

std::shared_ptr<pbft_operation>
pbft::find_operation(uint64_t view, uint64_t sequence, const pbft_request& request)
{
    auto key = bzn::operation_key_t(view, sequence, pbft_operation::request_hash(request));

    auto lookup = operations.find(key);
    if (lookup == operations.end())
    {
        LOG(debug) << "Creating operation for seq " << sequence << " view " << view << " req "
                   << request.ShortDebugString();

        std::shared_ptr<pbft_operation> op = std::make_shared<pbft_operation>(view, sequence, request,
                std::make_shared<std::vector<peer_address_t>>(this->peer_index));
        auto result = operations.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple(op));

        assert(result.second);
        return result.first->second;
    }

    return lookup->second;
}

bzn::message
pbft::wrap_message(const pbft_msg& msg, const std::string& debug_info)
{
    bzn::message json;

    json["bzn-api"] = "pbft";
    json["pbft-data"] = boost::beast::detail::base64_encode(msg.SerializeAsString());
    if(debug_info.length() > 0)
    {
        json["debug-info"] = debug_info;
    }

    return json;
}

bzn::message
pbft::wrap_message(const audit_message& msg, const std::string& debug_info)
{
    bzn::message json;

    json["bzn-api"] = "audit";
    json["audit-data"] = boost::beast::detail::base64_encode(msg.SerializeAsString());
    if(debug_info.length() > 0)
    {
        json["debug-info"] = debug_info;
    }

    return json;
}

const bzn::uuid_t&
pbft::get_uuid() const
{
    return this->uuid;
}

void
pbft::set_audit_enabled(bool setting)
{
    this->audit_enabled = setting;
}

void
pbft::notify_audit_failure_detected()
{
    if(this->audit_enabled)
    {
        audit_message msg;
        msg.mutable_failure_detected()->set_sender_uuid(this->uuid);
        this->broadcast(this->wrap_message(msg));
    }
}

void
pbft::handle_failure()
{
    LOG(fatal) << "Failure detected; view changes not yet implemented\n";
    this->notify_audit_failure_detected();
    //TODO: KEP-332
}

void
pbft::handle_database_message(const bzn::message& json, std::shared_ptr<bzn::session_base> session)
{
    bzn_msg msg;
    database_response response;

    if (!json.isMember("msg"))
    {
        LOG(error) << "Invalid message: " << json.toStyledString().substr(0,MAX_MESSAGE_SIZE) << "...";
        response.mutable_resp()->set_error(bzn::MSG_INVALID_CRUD_COMMAND);
        session->send_message(std::make_shared<std::string>(response.SerializeAsString()), true);
        return;
    }

    if (!msg.ParseFromString(boost::beast::detail::base64_decode(json["msg"].asString())))
    {
        LOG(error) << "Failed to decode message: " << json.toStyledString().substr(0,MAX_MESSAGE_SIZE) << "...";
        response.mutable_resp()->set_error(bzn::MSG_INVALID_CRUD_COMMAND);
        session->send_message(std::make_shared<std::string>(response.SerializeAsString()), true);
        return;
    }

    *response.mutable_header() = msg.db().header();

    pbft_request req;
    req.set_operation(json.toStyledString());
    req.set_timestamp(0); //TODO: KEP-611

    this->handle_request(req, session);

    session->send_message(std::make_shared<std::string>(response.SerializeAsString()), false);
}
