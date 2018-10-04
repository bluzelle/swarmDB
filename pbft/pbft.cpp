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
#include <boost/beast/core/detail/base64.hpp>
#include <boost/format.hpp>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <crud/crud_base.hpp>

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

    // TODO: stable checkpoint should be read from disk first: KEP-494
    this->low_water_mark = this->stable_checkpoint.first;
    this->high_water_mark = this->stable_checkpoint.first + std::lround(CHECKPOINT_INTERVAL*HIGH_WATER_INTERVAL_IN_CHECKPOINTS);
}

void
pbft::start()
{
    std::call_once(this->start_once,
            [this]()
            {
                this->node->register_for_message(bzn_msg_type::BZN_MSG_PBFT,
                        std::bind(&pbft::handle_bzn_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                this->node->register_for_message("database",
                        std::bind(&pbft::handle_database_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                this->audit_heartbeat_timer->expires_from_now(HEARTBEAT_INTERVAL);
                this->audit_heartbeat_timer->async_wait(
                        std::bind(&pbft::handle_audit_heartbeat_timeout, shared_from_this(), std::placeholders::_1));

                this->service->register_execute_handler(
                        [weak_this = this->weak_from_this(), fd = this->failure_detector]
                                (const pbft_request& req, uint64_t sequence)
                                        {
                                            fd->request_executed(req);

                                            if (sequence % CHECKPOINT_INTERVAL == 0)
                                            {
                                                auto strong_this = weak_this.lock();
                                                if(strong_this)
                                                {
                                                    strong_this->checkpoint_reached_locally(sequence);
                                                }
                                                else
                                                {
                                                    throw std::runtime_error("pbft_service callback failed because pbft does not exist");
                                                }
                                            }
                                        }
                );

                this->failure_detector->register_failure_handler(
                        [weak_this = this->weak_from_this()]()
                        {
                            auto strong_this = weak_this.lock();
                            if (strong_this)
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

    this->audit_heartbeat_timer->expires_from_now(HEARTBEAT_INTERVAL);
    this->audit_heartbeat_timer->async_wait(std::bind(&pbft::handle_audit_heartbeat_timeout, shared_from_this(), std::placeholders::_1));
}

void
pbft::handle_bzn_message(const wrapped_bzn_msg& msg, std::shared_ptr<bzn::session_base> /*session*/)
{
    if (msg.type() != BZN_MSG_PBFT)
    {
        LOG(error) << "Got misdirected message " << msg.DebugString().substr(0, MAX_MESSAGE_SIZE);
    }

    pbft_msg inner_msg;

    if (!inner_msg.ParseFromString(msg.payload()))
    {
        LOG(error) << "Failed to parse payload of wrapped message " << msg.DebugString().substr(0, MAX_MESSAGE_SIZE);
        return;
    }

    this->handle_message(inner_msg);
}

void
pbft::handle_message(const pbft_msg& msg)
{

    LOG(debug) << "Received message: " << msg.ShortDebugString().substr(0, MAX_MESSAGE_SIZE);

    if (!this->preliminary_filter_msg(msg))
    {
        return;
    }

    if (msg.has_request())
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
        case PBFT_MSG_CHECKPOINT :
            this->handle_checkpoint(msg);
            break;
        default :
            throw std::runtime_error("Unsupported message type");
    }
}

bool
pbft::preliminary_filter_msg(const pbft_msg& msg)
{
    auto t = msg.type();
    if (t == PBFT_MSG_PREPREPARE || t == PBFT_MSG_PREPARE || t == PBFT_MSG_COMMIT)
    {
        if (msg.view() != this->view)
        {
            LOG(debug) << "Dropping message because it has the wrong view number";
            return false;
        }

        if (msg.sequence() <= this->low_water_mark)
        {
            LOG(debug) << "Dropping message becasue it has an unreasonable sequence number " << msg.sequence();
            return false;
        }

        if (msg.sequence() > this->high_water_mark)
        {
            LOG(debug) << "Dropping message becasue it has an unreasonable sequence number " << msg.sequence();
            return false;
        }
    }

    return true;
}

void
pbft::handle_request(const pbft_request& msg, const std::shared_ptr<session_base>& session)
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

    if (session)
    {
        op->set_session(session);
    }

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
        auto op = this->find_operation(msg);
        op->record_preprepare();

        // This assignment will be redundant if we've seen this preprepare before, but that's fine
        accepted_preprepares[log_key] = op->get_operation_key();

        this->do_preprepared(op);
        this->maybe_advance_operation_state(op);
    }
}

void
pbft::handle_prepare(const pbft_msg& msg)
{

    // Prepare messages are never rejected, assuming the sanity checks passed
    auto op = this->find_operation(msg);

    op->record_prepare(msg);
    this->maybe_advance_operation_state(op);
}

void
pbft::handle_commit(const pbft_msg& msg)
{

    // Commit messages are never rejected, assuming  the sanity checks passed
    auto op = this->find_operation(msg);

    op->record_commit(msg);
    this->maybe_advance_operation_state(op);
}

void
pbft::broadcast(const bzn::encoded_message& msg)
{
    auto msg_ptr = std::make_shared<bzn::encoded_message>(msg);

    for (const auto& peer : this->peer_index)
    {
        this->node->send_message_str(make_endpoint(peer), msg_ptr);
    }
}

void
pbft::maybe_advance_operation_state(const std::shared_ptr<pbft_operation>& op)
{
    if (op->get_state() == pbft_operation_state::prepare && op->is_prepared())
    {
        this->do_prepared(op);
    }

    if (op->get_state() == pbft_operation_state::commit && op->is_committed())
    {
        this->do_committed(op);
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
pbft::do_preprepare(const std::shared_ptr<pbft_operation>& op)
{
    LOG(debug) << "Doing preprepare for operation " << op->debug_string();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPREPARE);

    this->broadcast(this->wrap_message(msg, "preprepare"));
}

void
pbft::do_preprepared(const std::shared_ptr<pbft_operation>& op)
{
    LOG(debug) << "Entering prepare phase for operation " << op->debug_string();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPARE);

    this->broadcast(this->wrap_message(msg, "prepare"));
}

void
pbft::do_prepared(const std::shared_ptr<pbft_operation>& op)
{
    LOG(debug) << "Entering commit phase for operation " << op->debug_string();
    op->begin_commit_phase();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_COMMIT);

    this->broadcast(this->wrap_message(msg, "commit"));
}

void
pbft::do_committed(const std::shared_ptr<pbft_operation>& op)
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

    // Get a new shared pointer to the operation so that we can give pbft_service ownership on it
    this->io_context->post(std::bind(&pbft_service_base::apply_operation, this->service, this->find_operation(op)));
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
pbft::find_operation(const std::shared_ptr<pbft_operation>& op)
{
    return this->find_operation(op->view, op->sequence, op->request);
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

bzn::encoded_message
pbft::wrap_message(const pbft_msg& msg, const std::string& /*debug_info*/)
{
    wrapped_bzn_msg result;
    result.set_payload(msg.SerializeAsString());
    result.set_type(bzn_msg_type::BZN_MSG_PBFT);

    return result.SerializeAsString();
}

bzn::encoded_message
pbft::wrap_message(const audit_message& msg, const std::string& debug_info)
{
    bzn::json_message json;

    json["bzn-api"] = "audit";
    json["audit-data"] = boost::beast::detail::base64_encode(msg.SerializeAsString());
    if (debug_info.length() > 0)
    {
        json["debug-info"] = debug_info;
    }

    return json.toStyledString();
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
    if (this->audit_enabled)
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
pbft::checkpoint_reached_locally(uint64_t sequence)
{

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    LOG(info) << "Reached checkpoint " << sequence;

    auto cp = this->local_unstable_checkpoints.emplace(sequence, this->service->service_state_hash(sequence)).first;

    pbft_msg cp_msg;
    cp_msg.set_type(PBFT_MSG_CHECKPOINT);
    cp_msg.set_view(this->view);
    cp_msg.set_sequence(sequence);
    cp_msg.set_sender(this->uuid);
    cp_msg.set_state_hash(cp->second);

    this->broadcast(this->wrap_message(cp_msg));

    this->maybe_stabilize_checkpoint(*cp);
}

void
pbft::handle_checkpoint(const pbft_msg& msg)
{
    if (msg.sequence() <= stable_checkpoint.first)
    {
        LOG(debug) << boost::format("Ignoring checkpoint message for seq %1% because I already have a stable checkpoint at seq %2%")
                   % msg.sequence()
                   % stable_checkpoint.first;
        return;
    }

    LOG(info) << boost::format("Recieved checkpoint message for seq %1% from %2%")
              % msg.sequence()
              % msg.sender();

    checkpoint_t cp(msg.sequence(), msg.state_hash());

    this->unstable_checkpoint_proofs[cp][msg.sender()] = msg.SerializeAsString();
    this->maybe_stabilize_checkpoint(cp);
}

bzn::checkpoint_t
pbft::latest_stable_checkpoint() const
{
    return this->stable_checkpoint;
}

bzn::checkpoint_t
pbft::latest_checkpoint() const
{
    return this->local_unstable_checkpoints.empty() ? this->stable_checkpoint : *(this->local_unstable_checkpoints.rbegin());
}

size_t
pbft::unstable_checkpoints_count() const
{
    return this->local_unstable_checkpoints.size();
}

void
pbft::maybe_stabilize_checkpoint(const checkpoint_t& cp)
{
    if (this->local_unstable_checkpoints.count(cp) == 0 || this->unstable_checkpoint_proofs[cp].size() < this->quorum_size())
    {
        return;
    }

    this->stable_checkpoint = cp;
    this->stable_checkpoint_proof = this->unstable_checkpoint_proofs[cp];

    LOG(info) << boost::format("Checkpoint %1% at seq %2% is now stable; clearing old data")
              % cp.second
              % cp.first;

    this->clear_local_checkpoints_until(cp);
    this->clear_checkpoint_messages_until(cp);
    this->clear_operations_until(cp);

    this->low_water_mark = std::max(this->low_water_mark, cp.first);
    this->high_water_mark = std::max(this->high_water_mark, cp.first + std::lround(HIGH_WATER_INTERVAL_IN_CHECKPOINTS*CHECKPOINT_INTERVAL));
}

void
pbft::clear_local_checkpoints_until(const checkpoint_t& cp)
{
    const auto local_start = this->local_unstable_checkpoints.begin();
    // Iterator to the first unstable checkpoint that's newer than this one. This logic assumes that CHECKPOINT_INTERVAL
    // is >= 2, otherwise we would have do do something awkward here
    const auto local_end = this->local_unstable_checkpoints.upper_bound(checkpoint_t(cp.first+1, ""));
    const size_t local_removed = std::distance(local_start, local_end);
    this->local_unstable_checkpoints.erase(local_start, local_end);
    LOG(debug) << boost::format("Cleared %1% unstable local checkpoints") % local_removed;
}

void
pbft::clear_checkpoint_messages_until(const checkpoint_t& cp)
{
    const auto start = this->unstable_checkpoint_proofs.begin();
    const auto end = this->unstable_checkpoint_proofs.upper_bound(checkpoint_t(cp.first+1, ""));
    const size_t to_remove = std::distance(start, end);
    this->unstable_checkpoint_proofs.erase(start, end);
    LOG(debug) << boost::format("Cleared %1% unstable checkpoint proof sets") % to_remove;
}

void
pbft::clear_operations_until(const checkpoint_t& cp)
{
    size_t ops_removed = 0;
    auto it = this->operations.begin();
    while (it != this->operations.end())
    {
        if(it->second->sequence <= cp.first)
        {
            it = this->operations.erase(it);
            ops_removed++;
        }
        else
        {
            it++;
        }
    }

    LOG(debug) << boost::format("Cleared %1% old operation records") % ops_removed;
}

size_t
pbft::quorum_size() const
{
    return 1 + (2*this->max_faulty_nodes());
}

size_t
pbft::max_faulty_nodes() const
{
    return this->peer_index.size()/3;
}

void
pbft::handle_database_message(const bzn::json_message& json, std::shared_ptr<bzn::session_base> session)
{
    bzn_msg msg;
    database_response response;

    LOG(debug) << "got database message: " << json.toStyledString();

    if (!json.isMember("msg"))
    {
        LOG(error) << "Invalid message: " << json.toStyledString().substr(0,MAX_MESSAGE_SIZE) << "...";
        response.mutable_error()->set_message(bzn::MSG_INVALID_CRUD_COMMAND);
        session->send_message(std::make_shared<bzn::encoded_message>(response.SerializeAsString()), true);
        return;
    }

    if (!msg.ParseFromString(boost::beast::detail::base64_decode(json["msg"].asString())))
    {
        LOG(error) << "Failed to decode message: " << json.toStyledString().substr(0,MAX_MESSAGE_SIZE) << "...";
        response.mutable_error()->set_message(bzn::MSG_INVALID_CRUD_COMMAND);
        session->send_message(std::make_shared<bzn::encoded_message>(response.SerializeAsString()), true);
        return;
    }

    *response.mutable_header() = msg.db().header();

    pbft_request req;
    *req.mutable_operation() = msg.db();
    req.set_timestamp(0); //TODO: KEP-611

    this->handle_request(req, session);

    LOG(debug) << "Sending request ack: " << response.ShortDebugString();
    session->send_message(std::make_shared<bzn::encoded_message>(response.SerializeAsString()), false);
}

uint64_t
pbft::get_low_water_mark()
{
    return this->low_water_mark;
}

uint64_t
pbft::get_high_water_mark()
{
    return this->high_water_mark;
}

std::string
pbft::get_name()
{
    return "pbft";
}


bzn::json_message
pbft::get_status()
{
    bzn::json_message status;

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    status["outstanding_operations_count"] = uint64_t(this->outstanding_operations_count());
    status["is_primary"] = this->is_primary();

    auto primary = this->get_primary();
    status["primary"]["host"] = primary.host;
    status["primary"]["host_port"] = primary.port;
    status["primary"]["http_port"] = primary.http_port;
    status["primary"]["name"] = primary.name;
    status["primary"]["uuid"] = primary.uuid;

    status["latest_stable_checkpoint"]["count"] = this->latest_stable_checkpoint().first;
    status["latest_stable_checkpoint"]["hash"] = this->latest_stable_checkpoint().second;
    status["latest_checkpoint"]["count"] = this->latest_checkpoint().first;
    status["latest_checkpoint"]["hash"] = this->latest_checkpoint().first;

    status["unstable_checkpoints_count"] = uint64_t(this->unstable_checkpoints_count());
    status["next_issued_sequence_number"] = this->next_issued_sequence_number;
    status["view"] = this->view;

    status["peer_index"] = bzn::json_message();
    for(const auto& p : this->peer_index)
    {
        bzn::json_message peer;
        peer["host"] = p.host;
        peer["port"] = p.port;
        peer["http_port"] = p.http_port;
        peer["name"] = p.name;
        peer["uuid"] = p.uuid;
        status["peer_index"].append(peer);
    }

    return status;
}

