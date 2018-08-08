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
)
        : node(std::move(node))
        , uuid(std::move(uuid))
        , service(service)
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
                        std::bind(&pbft::unwrap_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                this->audit_heartbeat_timer->expires_from_now(heartbeat_interval);
                this->audit_heartbeat_timer->async_wait(
                        std::bind(&pbft::handle_audit_heartbeat_timeout, shared_from_this(), std::placeholders::_1));
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
pbft::unwrap_message(const bzn::message& json, std::shared_ptr<bzn::session_base> /*session*/)
{
    pbft_msg msg;
    if (msg.ParseFromString(boost::beast::detail::base64_decode(json["pbft-data"].asString())))
    {
        this->handle_message(msg);
    }
    LOG(error) << "Got invalid message " << json.toStyledString().substr(0, MAX_MESSAGE_SIZE);
}

void
pbft::handle_message(const pbft_msg& msg)
{

    LOG(debug) << "Received message: " << msg.ShortDebugString().substr(0, MAX_MESSAGE_SIZE);

    if (!this->preliminary_filter_msg(msg))
    {
        return;
    }

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    switch (msg.type())
    {
        case PBFT_MSG_REQUEST :
            this->handle_request(msg);
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
pbft::handle_request(const pbft_msg& msg)
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
    pbft_operation& op = this->find_operation(this->view, request_seq, msg.request());

    this->do_preprepare(op);
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
        pbft_operation& op = this->find_operation(msg);
        op.record_preprepare();

        // This assignment will be redundant if we've seen this preprepare before, but that's fine
        accepted_preprepares[log_key] = op.get_operation_key();

        this->do_preprepared(op);
        this->maybe_advance_operation_state(op);
    }
}

void
pbft::handle_prepare(const pbft_msg& msg)
{

    // Prepare messages are never rejected, assuming the sanity checks passed
    pbft_operation& op = this->find_operation(msg);

    op.record_prepare(msg);
    this->maybe_advance_operation_state(op);
}

void
pbft::handle_commit(const pbft_msg& msg)
{

    // Commit messages are never rejected, assuming  the sanity checks passed
    pbft_operation& op = this->find_operation(msg);

    op.record_commit(msg);
    this->maybe_advance_operation_state(op);
}

void
pbft::broadcast(const bzn::message& json)
{
    auto json_ptr = std::make_shared<bzn::message>(json);

    for (const auto& peer : this->peer_index)
    {
        this->node->send_message(make_endpoint(peer), json_ptr);
    }
}

void
pbft::maybe_advance_operation_state(pbft_operation& op)
{
    if (op.get_state() == pbft_operation_state::prepare && op.is_prepared())
    {
        this->do_prepared(op);
    }

    if (op.get_state() == pbft_operation_state::commit && op.is_committed())
    {
        this->do_committed(op);
    }
}

pbft_msg
pbft::common_message_setup(const pbft_operation& op, pbft_msg_type type)
{
    pbft_msg msg;
    msg.set_view(op.view);
    msg.set_sequence(op.sequence);
    msg.set_allocated_request(new pbft_request(op.request));
    msg.set_type(type);


    // Some message types don't need this, but it's cleaner to always include it
    msg.set_sender(this->uuid);

    return msg;
}

void
pbft::do_preprepare(pbft_operation& op)
{
    LOG(debug) << "Doing preprepare for operation " << op.debug_string();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPREPARE);

    this->broadcast(this->wrap_message(msg));
}

void
pbft::do_preprepared(pbft_operation& op)
{
    LOG(debug) << "Entering prepare phase for operation " << op.debug_string();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPARE);

    this->broadcast(this->wrap_message(msg));
}

void
pbft::do_prepared(pbft_operation& op)
{
    LOG(debug) << "Entering commit phase for operation " << op.debug_string();
    op.begin_commit_phase();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_COMMIT);

    this->broadcast(this->wrap_message(msg));
}

void
pbft::do_committed(pbft_operation& op)
{
    LOG(debug) << "Operation " << op.debug_string() << " is committed-local";
    op.end_commit_phase();

    this->service->commit_request(op.sequence, op.request);

    if (this->audit_enabled)
    {
        audit_message msg;
        msg.mutable_pbft_commit()->set_operation(pbft_operation::request_hash(op.request));
        msg.mutable_pbft_commit()->set_sequence_number(op.sequence);
        msg.mutable_pbft_commit()->set_sender_uuid(this->uuid);

        this->broadcast(this->wrap_message(msg));
    }
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
pbft_operation&
pbft::find_operation(const pbft_msg& msg)
{
    return this->find_operation(msg.view(), msg.sequence(), msg.request());
}

pbft_operation&
pbft::find_operation(uint64_t view, uint64_t sequence, const pbft_request& request)
{
    auto key = bzn::operation_key_t(view, sequence, pbft_operation::request_hash(request));

    auto lookup = operations.find(key);
    if (lookup == operations.end())
    {
        LOG(debug) << "Creating operation for seq " << sequence << " view " << view << " req "
                   << request.ShortDebugString();
        auto result = operations.emplace(
                std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(
                        view
                        , sequence
                        , request
                        , std::make_shared<std::vector<peer_address_t>>(this->peer_index)
                ));
        assert(result.second);
        return result.first->second;
    }

    return lookup->second;
}

bzn::message
pbft::wrap_message(const pbft_msg& msg)
{
    bzn::message json;

    json["bzn-api"] = "pbft";
    json["pbft-data"] = boost::beast::detail::base64_encode(msg.SerializeAsString());

    return json;
}

bzn::message
pbft::wrap_message(const audit_message& msg)
{
    bzn::message json;

    json["bzn-api"] = "audit";
    json["audit-data"] = boost::beast::detail::base64_encode(msg.SerializeAsString());

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