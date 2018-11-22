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
#include <utils/bytes_to_debug_string.hpp>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/message_differencer.h>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/format.hpp>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <numeric>
#include <optional>
#include <iterator>
#include <crud/crud_base.hpp>
#include <random>
#include <chrono>

using namespace bzn;

pbft::pbft(
    std::shared_ptr<bzn::node_base> node
    , std::shared_ptr<bzn::asio::io_context_base> io_context
    , const bzn::peers_list_t& peers
    , bzn::uuid_t uuid
    , std::shared_ptr<pbft_service_base> service
    , std::shared_ptr<pbft_failure_detector_base> failure_detector
    , std::shared_ptr<bzn::crypto_base> crypto
    )
    : node(std::move(node))
    , uuid(std::move(uuid))
    , service(std::move(service))
    , failure_detector(std::move(failure_detector))
    , io_context(io_context)
    , audit_heartbeat_timer(this->io_context->make_unique_steady_timer()), crypto(std::move(crypto))
{
    if (peers.empty())
    {
        throw std::runtime_error("No peers found!");
    }

    this->initialize_configuration(peers);

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
                this->node->register_for_message(bzn_envelope::PayloadCase::kPbft,
                        std::bind(&pbft::handle_bzn_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                this->node->register_for_message(bzn_envelope::PayloadCase::kPbftMembership,
                    std::bind(&pbft::handle_membership_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                this->node->register_for_message(bzn_envelope::kDatabaseMsg,
                        std::bind(&pbft::handle_database_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

                this->audit_heartbeat_timer->expires_from_now(HEARTBEAT_INTERVAL);
                this->audit_heartbeat_timer->async_wait(
                        std::bind(&pbft::handle_audit_heartbeat_timeout, shared_from_this(), std::placeholders::_1));

                this->service->register_execute_handler(
                        [weak_this = this->weak_from_this(), fd = this->failure_detector]
                                (std::shared_ptr<pbft_operation> op)
                                        {
                                            if (!op)
                                            {
                                                // TODO: Get real pbft_operation pointers from pbft_service
                                                LOG(error) << "Ignoring null operation pointer recieved from pbft_service";
                                                return;
                                            }

                                            fd->request_executed(op->request_hash);

                                            if (op->sequence % CHECKPOINT_INTERVAL == 0)
                                            {
                                                auto strong_this = weak_this.lock();
                                                if (strong_this)
                                                {
                                                    strong_this->checkpoint_reached_locally(op->sequence);
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
pbft::handle_bzn_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> /*session*/)
{
    if (msg.payload_case() != bzn_envelope::kPbft )
    {
        LOG(error) << "Got misdirected message " << msg.DebugString().substr(0, MAX_MESSAGE_SIZE);
    }

    pbft_msg inner_msg;
    if (!inner_msg.ParseFromString(msg.pbft()))
    {
        LOG(error) << "Failed to parse payload of wrapped message " << msg.DebugString().substr(0, MAX_MESSAGE_SIZE);
        return;
    }

    this->handle_message(inner_msg, msg);
}

void
pbft::handle_membership_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session)
{
    pbft_membership_msg inner_msg;
    if (!inner_msg.ParseFromString(msg.pbft_membership()))
    {
        LOG(error) << "Failed to parse payload of wrapped message " << msg.DebugString().substr(0, MAX_MESSAGE_SIZE);
        return;
    }

    switch (inner_msg.type())
    {
        case PBFT_MMSG_JOIN:
        case PBFT_MMSG_LEAVE:
            this->handle_join_or_leave(inner_msg);
            break;
        case PBFT_MMSG_GET_STATE:
            this->handle_get_state(inner_msg, std::move(session));
            break;
        case PBFT_MMSG_SET_STATE:
            this->handle_set_state(inner_msg);
            break;
        default:
            LOG(error) << "Invalid membership message received "
                << inner_msg.DebugString().substr(0, MAX_MESSAGE_SIZE);
    }
}

void
pbft::handle_message(const pbft_msg& msg, const bzn_envelope& original_msg)
{

    LOG(debug) << "Received message: " << msg.ShortDebugString().substr(0, MAX_MESSAGE_SIZE);

    if (!this->preliminary_filter_msg(msg))
    {
        return;
    }

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    switch (msg.type())
    {
        case PBFT_MSG_PREPREPARE :
            this->handle_preprepare(msg, original_msg);
            break;
        case PBFT_MSG_PREPARE :
            this->handle_prepare(msg, original_msg);
            break;
        case PBFT_MSG_COMMIT :
            this->handle_commit(msg, original_msg);
            break;
        case PBFT_MSG_CHECKPOINT :
            this->handle_checkpoint(msg, original_msg);
            break;
        case PBFT_MSG_VIEWCHANGE :
            this->handle_viewchange(msg, original_msg);
            break;
        case PBFT_MSG_NEWVIEW :
            this->handle_newview(msg, original_msg);
            break;

        default :
            throw std::runtime_error("Unsupported message type");
    }
}

bool
pbft::preliminary_filter_msg(const pbft_msg& msg)
{
    if (
            !this->is_view_valid()
            && ( msg.type() != PBFT_MSG_CHECKPOINT || msg.type() != PBFT_MSG_VIEWCHANGE || msg.type() != PBFT_MSG_NEWVIEW) )
    {
        LOG(debug) << "Dropping message because local view is invalid";
        return false;
    }

    auto t = msg.type();
    if (
            t == PBFT_MSG_PREPREPARE
            || t == PBFT_MSG_PREPARE
            || t == PBFT_MSG_COMMIT)
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

std::shared_ptr<pbft_operation>
pbft::setup_request_operation(const bzn_envelope& request_env, const bzn::hash_t& request_hash, const std::shared_ptr<session_base>& session)
{
    const uint64_t request_seq = this->next_issued_sequence_number++;
    auto op = this->find_operation(this->view, request_seq, request_hash);
    op->record_request(request_env);

    if (session)
    {
        op->set_session(session);
    }

    return op;
}

void
pbft::handle_request(const bzn_envelope& request_env, const std::shared_ptr<session_base>& session)
{
    if (!this->is_primary())
    {
        LOG(info) << "Forwarding request to primary";
        this->node->send_message(bzn::make_endpoint(this->get_primary()), std::make_shared<bzn_envelope>(request_env));

        const bzn::hash_t req_hash = this->crypto->hash(request_env);

        const auto existing_operation = std::find_if(this->operations.begin(), this->operations.end(),
            // This search is inefficient for in-memory operations, but the db lookup that will replace it is not
            [&](const auto& pair)
            {
                return std::get<2>(pair.first) == req_hash;
            });

        // If we already have an operation for this request_hash, attach the session to it
        if (existing_operation != this->operations.end())
        {
            const std::shared_ptr<bzn::pbft_operation>& op = existing_operation->second;
            LOG(debug) << "We already had an operation for that request hash; attaching session to it";

            if (!op->has_session())
            {
                op->set_session(session);
            }
        }
        else
        {
            LOG(debug) << "Saving session because we don't have an operation for this hash: " << bzn::bytes_to_debug_string(req_hash);
            this->sessions_waiting_on_forwarded_requests[req_hash] = session;
        }

        this->failure_detector->request_seen(req_hash);

        return;
    }

    if (request_env.timestamp() < (this->now() - MAX_REQUEST_AGE_MS) || request_env.timestamp() > (this->now() + MAX_REQUEST_AGE_MS))
    {
        // TODO: send error message to client
        LOG(info) << "Rejecting request because it is outside allowable timestamp range: " << request_env.ShortDebugString();
        return;
    }

    auto hash = this->crypto->hash(request_env);

    // keep track of what requests we've seen based on timestamp and only send preprepares once
    if (this->already_seen_request(request_env, hash))
    {
        // TODO: send error message to client
        LOG(info) << "Rejecting duplicate request: " << request_env.ShortDebugString();
        return;
    }
    this->saw_request(request_env, hash);
    auto op = setup_request_operation(request_env, hash, session);
    this->do_preprepare(op);
}

void
pbft::maybe_record_request(const pbft_msg& msg, const std::shared_ptr<pbft_operation>& op)
{
    if (msg.has_request() && !op->has_request())
    {
        if (this->crypto->hash(msg.request()) != msg.request_hash())
        {
            LOG(info) << "Not recording request because its hash does not match";
            return;
        }

        op->record_request(msg.request());
    }
}

void
pbft::handle_preprepare(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    // If we've already accepted a preprepare for this view+sequence, and it's not this one, then we should reject this one
    // Note that if we get the same preprepare more than once, we can still accept it
    const log_key_t log_key(msg.view(), msg.sequence());

    if (auto lookup = this->accepted_preprepares.find(log_key);
        lookup != this->accepted_preprepares.end()
        && std::get<2>(lookup->second) != msg.request_hash())
    {

        LOG(debug) << "Rejecting preprepare because I've already accepted a conflicting one \n";
        return;
    }
    else
    {
        auto op = this->find_operation(msg);
        op->record_preprepare(original_msg);
        this->maybe_record_request(msg, op);

        // This assignment will be redundant if we've seen this preprepare before, but that's fine
        accepted_preprepares[log_key] = op->get_operation_key();

        if (op->has_config_request())
        {
            this->handle_config_message(msg, op);
        }

        this->do_preprepared(op);
        this->maybe_advance_operation_state(op);
    }
}

void
pbft::handle_prepare(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    // Prepare messages are never rejected, assuming the sanity checks passed
    auto op = this->find_operation(msg);

    op->record_prepare(original_msg);
    this->maybe_record_request(msg, op);
    this->maybe_advance_operation_state(op);
}

void
pbft::handle_commit(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    // Commit messages are never rejected, assuming  the sanity checks passed
    auto op = this->find_operation(msg);

    op->record_commit(original_msg);
    this->maybe_record_request(msg, op);
    this->maybe_advance_operation_state(op);
}

void
pbft::handle_join_or_leave(const pbft_membership_msg& msg)
{
    if (!this->is_primary())
    {
        LOG(error) << "Ignoring client request because I am not the primary";
        // TODO - KEP-327
        return;
    }

    if (msg.has_peer_info())
    {
        // build a peer_address_t from the message
        auto const &peer_info = msg.peer_info();
        bzn::peer_address_t peer(peer_info.host(), static_cast<uint16_t>(peer_info.port()),
            static_cast<uint16_t>(peer_info.http_port()), peer_info.name(), peer_info.uuid());

        auto config = std::make_shared<pbft_configuration>(*(this->configurations.current()));
        if (msg.type() == PBFT_MMSG_JOIN)
        {
            // see if we can add this peer
            if (!config->add_peer(peer))
            {
                // TODO - respond with negative result?
                LOG(debug) << "Can't add new peer due to conflict";
                return;
            }
        }
        else if (msg.type() == PBFT_MMSG_LEAVE)
        {
            if (!config->remove_peer(peer))
            {
                // TODO - respond with negative result?
                LOG(debug) << "Couldn't remove requested peer";
                return;
            }
        }

        this->configurations.add(config);
        this->broadcast_new_configuration(config);
    }
    else
    {
        LOG(debug) << "Malformed join/leave message";
    }
}

void
pbft::handle_get_state(const pbft_membership_msg& msg, std::shared_ptr<bzn::session_base> session) const
{
    // get stable checkpoint for request
    checkpoint_t req_cp(msg.sequence(), msg.state_hash());

    if (req_cp == this->latest_stable_checkpoint())
    {
        pbft_membership_msg reply;
        reply.set_type(PBFT_MMSG_SET_STATE);
        reply.set_sequence(req_cp.first);
        reply.set_state_hash(req_cp.second);
        reply.set_state_data(this->get_checkpoint_state(req_cp));

        auto msg_ptr = std::make_shared<bzn::encoded_message>(this->wrap_message(reply).SerializeAsString());
        session->send_datagram(msg_ptr);
    }
    else
    {
        LOG(debug) << boost::format("Request for checkpoint that I don't have: seq: %1%, hash: %2%")
            % msg.sequence(), msg.state_hash();
    }
}

void
pbft::handle_set_state(const pbft_membership_msg& msg)
{
    checkpoint_t cp(msg.sequence(), msg.state_hash());

    // do we need this checkpoint state?
    // make sure we don't have this checkpoint locally, but do know of a stablized one
    // based on the messages sent by peers.
    if (this->unstable_checkpoint_proofs[cp].size() >= this->quorum_size() &&
        this->local_unstable_checkpoints.count(cp) == 0)
    {
        LOG(info) << boost::format("Adopting checkpoint %1% at seq %2%")
            % cp.second % cp.first;

        this->set_checkpoint_state(cp, msg.state_data());
        this->stabilize_checkpoint(cp);
    }
    else
    {
        LOG(debug) << boost::format("Sent state for checkpoint that I don't need: seq: %1%, hash: %2%")
            % msg.sequence() % msg.state_hash();
    }
}

void
pbft::broadcast(const bzn_envelope& msg)
{
    auto msg_ptr = std::make_shared<bzn_envelope>(msg);

    for (const auto& peer : this->current_peers())
    {
        this->node->send_message(make_endpoint(peer), msg_ptr);
    }
}

void
pbft::broadcast(const bzn::encoded_message& msg)
{
    // broadcast(bzn_envelope) is preferred, this is kept for now because audit still uses json wrapping

    auto msg_ptr = std::make_shared<bzn::encoded_message>(msg);

    for (const auto& peer : this->current_peers())
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
    msg.set_request_hash(op->request_hash);
    msg.set_type(type);

    return msg;
}

void
pbft::do_preprepare(const std::shared_ptr<pbft_operation>& op)
{
    LOG(debug) << "Doing preprepare for operation " << op->debug_string();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPREPARE);
    msg.set_allocated_request(new bzn_envelope(op->get_request()));

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
    // accept new configuration if applicable
    if (op->has_config_request())
    {
        pbft_configuration config;
        if (config.from_string(op->get_config_request().configuration()))
        {
            this->configurations.enable(config.get_hash());
        }
    }

    LOG(debug) << "Entering commit phase for operation " << op->debug_string();
    op->begin_commit_phase();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_COMMIT);

    this->broadcast(this->wrap_message(msg, "commit"));
}

void
pbft::do_committed(const std::shared_ptr<pbft_operation>& op)
{
    // commit new configuration if applicable
    if (op->has_config_request())
    {
        pbft_configuration config;
        if (config.from_string(op->get_config_request().configuration()))
        {
            // get rid of all other previous configs, except for currently active one
            this->configurations.remove_prior_to(config.get_hash());
        }
    }

    LOG(debug) << "Operation " << op->debug_string() << " is committed-local";
    op->end_commit_phase();

    if (this->audit_enabled)
    {
        audit_message msg;
        msg.mutable_pbft_commit()->set_operation(op->request_hash);
        msg.mutable_pbft_commit()->set_sequence_number(op->sequence);
        msg.mutable_pbft_commit()->set_sender_uuid(this->uuid);

        this->broadcast(this->wrap_message(msg));
    }

    // TODO: this needs to be refactored to be service-agnostic
    if (op->has_db_request())
    {
        this->io_context->post(std::bind(&pbft_service_base::apply_operation, this->service, op));
    }
    else
    {
        // the service needs sequentially sequenced operations. post a null request to fill in this hole
        auto msg = new database_msg;
        msg->set_allocated_nullmsg(new database_nullmsg);
        bzn_envelope request;
        request.set_database_msg(msg->SerializeAsString());
        auto new_op = std::make_shared<pbft_operation>(op->view, op->sequence
            , this->crypto->hash(request), nullptr);
        new_op->record_request(request);
        this->io_context->post(std::bind(&pbft_service_base::apply_operation, this->service, new_op));
            request.SerializeAsString();
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
pbft::get_primary(std::optional<uint64_t> view ) const
{
    return this->current_peers()[view.value_or(this->view) % this->current_peers().size()];
}

// Find this node's record of an operation (creating a new record for it if this is the first time we've heard of it)
std::shared_ptr<pbft_operation>
pbft::find_operation(const pbft_msg& msg)
{
    return this->find_operation(msg.view(), msg.sequence(), msg.request_hash());
}

std::shared_ptr<pbft_operation>
pbft::find_operation(const std::shared_ptr<pbft_operation>& op)
{
    return this->find_operation(op->view, op->sequence, op->request_hash);
}

std::shared_ptr<pbft_operation>
pbft::find_operation(uint64_t view, uint64_t sequence, const bzn::hash_t& req_hash)
{
    auto key = bzn::operation_key_t(view, sequence, req_hash);

    auto lookup = operations.find(key);
    if (lookup == operations.end())
    {
        LOG(debug) << "Creating operation for seq " << sequence << " view " << view << " req " << bytes_to_debug_string(req_hash);

        std::shared_ptr<pbft_operation> op = std::make_shared<pbft_operation>(view, sequence, req_hash,
                this->current_peers_ptr());
        bool added;
        std::tie(std::ignore, added) = operations.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple(op));
        assert(added);

        auto session_pair = this->sessions_waiting_on_forwarded_requests.find(req_hash);
        if (session_pair != this->sessions_waiting_on_forwarded_requests.end())
        {
            LOG(debug) << "Attaching pending session to new operation";
            std::weak_ptr<bzn::session_base> session;
            std::tie(std::ignore, session) = *session_pair;
            op->set_session(session);
            this->sessions_waiting_on_forwarded_requests.erase(req_hash);
        }

        return op;
    }

    return lookup->second;
}

bzn_envelope
pbft::wrap_message(const pbft_msg& msg, const std::string& /*debug_info*/)
{
    bzn_envelope result;
    result.set_pbft(msg.SerializeAsString());
    result.set_sender(this->uuid);

    return result;
}

bzn_envelope
pbft::wrap_message(const pbft_membership_msg& msg, const std::string& /*debug_info*/) const
{
    bzn_envelope result;
    result.set_pbft_membership(msg.SerializeAsString());
    result.set_sender(this->uuid);

    return result;
}

bzn::encoded_message
pbft::wrap_message(const pbft_membership_msg& msg, const std::string& /*debug_info*/)
{
    wrapped_bzn_msg result;
    result.set_payload(msg.SerializeAsString());
    result.set_type(bzn_msg_type::BZN_MSG_PBFT_MEMBERSHIP);
    result.set_sender(this->uuid);

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
    this->notify_audit_failure_detected();
    this->view_is_valid = false;
    pbft_msg view_change
    {
            pbft::make_viewchange(
            this->view + 1
            , this->latest_stable_checkpoint().first
            , this->stable_checkpoint_proof
            , this->prepared_operations_since_last_checkpoint()
            )};
    this->broadcast(this->wrap_message(view_change));
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
    cp_msg.set_state_hash(cp->second);

    this->broadcast(this->wrap_message(cp_msg));

    this->maybe_stabilize_checkpoint(*cp);
}

void
pbft::handle_checkpoint(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    if (msg.sequence() <= stable_checkpoint.first)
    {
        LOG(debug) << boost::format("Ignoring checkpoint message for seq %1% because I already have a stable checkpoint at seq %2%")
                   % msg.sequence()
                   % stable_checkpoint.first;
        return;
    }

    LOG(info) << boost::format("Received checkpoint message for seq %1% from %2%")
              % msg.sequence()
              % original_msg.sender();

    checkpoint_t cp(msg.sequence(), msg.state_hash());

    this->unstable_checkpoint_proofs[cp][original_msg.sender()] = original_msg.SerializeAsString();
    if (msg.sequence() > this->first_sequence_to_execute)
    {
        this->maybe_stabilize_checkpoint(cp);
    }
    else
    {
        this->maybe_adopt_checkpoint(cp);
    }
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
    if (this->unstable_checkpoint_proofs[cp].size() < this->quorum_size())
    {
        return;
    }

    if (this->local_unstable_checkpoints.count(cp) != 0)
    {
        this->stabilize_checkpoint(cp);
    }
    else
    {
        // we don't have this checkpoint, so we need to catch up
        this->request_checkpoint_state(cp);
    }
}

void
pbft::stabilize_checkpoint(const checkpoint_t& cp)
{
    this->stable_checkpoint = cp;
    this->stable_checkpoint_proof = this->unstable_checkpoint_proofs[cp];

    LOG(info) << boost::format("Checkpoint %1% at seq %2% is now stable; clearing old data")
        % cp.second % cp.first;

    this->clear_local_checkpoints_until(cp);
    this->clear_checkpoint_messages_until(cp);
    this->clear_operations_until(cp);

    this->low_water_mark = std::max(this->low_water_mark, cp.first);
    this->high_water_mark = std::max(this->high_water_mark,
        cp.first + std::lround(HIGH_WATER_INTERVAL_IN_CHECKPOINTS * CHECKPOINT_INTERVAL));

    this->service->consolidate_log(cp.first);

    // remove seen requests older than our time threshold
    this->recent_requests.erase(this->recent_requests.begin(),
        this->recent_requests.upper_bound(this->now() - MAX_REQUEST_AGE_MS));
}

void
pbft::request_checkpoint_state(const checkpoint_t& cp)
{
    pbft_membership_msg msg;
    msg.set_type(PBFT_MMSG_GET_STATE);
    msg.set_sequence(cp.first);
    msg.set_state_hash(cp.second);

    auto selected = this->select_peer_for_checkpoint(cp);
    LOG(info) << boost::format("Requesting checkpoint state for hash %1% at seq %2% from %3%")
        % cp.second % cp.first % selected.uuid;

    auto msg_ptr = std::make_shared<bzn_envelope>();
    msg_ptr->set_pbft_membership(msg.SerializeAsString());


    this->node->send_message(make_endpoint(selected), msg_ptr);
}

const peer_address_t&
pbft::select_peer_for_checkpoint(const checkpoint_t& cp)
{
    // choose one of the peers who vouch for this checkpoint at random
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, this->unstable_checkpoint_proofs[cp].size() - 1);

    auto it = this->unstable_checkpoint_proofs[cp].begin();
    uint32_t selected = dist(gen);
    for (size_t i = 0; i < selected; i++)
    {
        it++;
    }

    return this->get_peer_by_uuid(it->first);
}

std::string
pbft::get_checkpoint_state(const checkpoint_t& cp) const
{
    // call service to retrieve state at this checkpoint
    return this->service->get_service_state(cp.first);
}

void
pbft::set_checkpoint_state(const checkpoint_t& cp, const std::string& data)
{
    // set the service state at the given checkpoint sequence
    // the service is expected to load the state and discard any pending operations
    // prior to the sequence number, then execute any subsequent operations sequentially
    this->service->set_service_state(cp.first, data);
}

void
pbft::maybe_adopt_checkpoint(const checkpoint_t& cp)
{
    if (this->unstable_checkpoint_proofs[cp].size() < this->quorum_size())
    {
        return;
    }

    pbft_membership_msg msg;
    msg.set_type(PBFT_MMSG_GET_STATE);
    msg.set_sequence(cp.first);
    msg.set_state_hash(cp.second);

    auto msg_ptr = std::make_shared<bzn::encoded_message>(this->wrap_message(msg));
    this->node->send_message_str(make_endpoint(this->get_primary()), msg_ptr);



//    this->stable_checkpoint = cp;
//    this->stable_checkpoint_proof = this->unstable_checkpoint_proofs[cp];
//
//    LOG(info) << boost::format("Checkpoint %1% at seq %2% is now stable; clearing old data")
//                 % cp.second
//                 % cp.first;
//
//    this->clear_local_checkpoints_until(cp);
//    this->clear_checkpoint_messages_until(cp);
//    this->clear_operations_until(cp);
//
//    this->low_water_mark = std::max(this->low_water_mark, cp.first);
//    this->high_water_mark = std::max(this->high_water_mark, cp.first + std::lround(HIGH_WATER_INTERVAL_IN_CHECKPOINTS*CHECKPOINT_INTERVAL));
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
        if (it->second->sequence <= cp.first)
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
    return this->current_peers().size()/3;
}

void
pbft::handle_database_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session)
{
    // TODO: timestamp should be set by the client. setting it here breaks the signature (which is correct).
    bzn_envelope mutable_msg(msg);
    if (msg.timestamp() == 0)
    {
        mutable_msg.set_timestamp(this->now());
    }

    LOG(debug) << "got database message";
    this->handle_request(mutable_msg, session);

    database_response response;
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

bool
pbft::is_view_valid() const
{
    return this->view_is_valid;
}


bool
pbft::is_peer(const bzn::uuid_t& sender) const
{
    return std::find_if (
            std::begin( this->current_peers())
            , std::end( this->current_peers())
            , [&](const auto& address)
            {
                return sender== address.uuid;
            }) != this->current_peers().end();

}


std::map<bzn::checkpoint_t , std::set<bzn::uuid_t>>
pbft::validate_and_extract_checkpoint_hashes(const pbft_msg &viewchange_message) const
{
    std::map<bzn::checkpoint_t , std::set<bzn::uuid_t>> checkpoint_hashes;
    for (size_t i{0} ; i < static_cast<uint64_t>(viewchange_message.checkpoint_messages_size()) ; ++i)
    {
        bzn_envelope envelope;
        pbft_msg checkpoint_message;
        if (
            !envelope.ParseFromString(viewchange_message.checkpoint_messages(i))
            || !this->crypto->verify(envelope)
            || !this->is_peer(envelope.sender())
            || !checkpoint_message.ParseFromString(envelope.pbft())
            )
        {
            LOG (error) << "Checkpoint validation failure - unable to parse envelope";
            continue;
        }

        checkpoint_hashes[checkpoint_t(
                checkpoint_message.sequence(),
                checkpoint_message.state_hash())].insert(envelope.sender());
    }
    return checkpoint_hashes;
}


/**
 * Given a VIEWCHANGE message this method ensures that there are 2f+1 from each
 * unique replica in the swarm that have the same checkpoint hash.
 *
 * @param viewchange_message
 * @return
 */
std::optional<bzn::checkpoint_t>
pbft::validate_viewchange_checkpoints(const pbft_msg &viewchange_message) const
{
    //with viewchange_message.checkpoint_messages
    //are there 2f+1 checkpoint messages each from a different nodes that are in the swarm
    // that have the same checkpoint hash and valid signatures
    size_t checkpoint_message_count = size_t(viewchange_message.checkpoint_messages_size());

    if ( checkpoint_message_count >= 2 * this->max_faulty_nodes() + 1 )
    {
        std::map<bzn::checkpoint_t , std::set<bzn::uuid_t>> checkpoint_hashes{
                this->validate_and_extract_checkpoint_hashes(viewchange_message)};

        const auto p = std::find_if(//  std::pair<bzn::checkpoint_t , std::set<bzn::uuid_t>>
                checkpoint_hashes.begin(),
                checkpoint_hashes.end(),
                [&](const auto& pair){ return pair.second.size() >= 2 * this->max_faulty_nodes() + 1;}
                );
        if (p != checkpoint_hashes.end())
        {
            return p->first;
        }
    }
    else
    {
        LOG(error) << "validate_viewchange_checkpoints - not a valid VIEWCHANGE message: less than (2f+1) checkpoint messages";
    }
    return std::nullopt;
    // Isabel: there needs to be validation here
    // Everything that the view-change message claims, it also provides proofs for. We need to check
    // all those proofs.
    // Also, I think we can view-change to views other than the immediately next one. Remember that we don't actually
    // do anything until we get 2f+1 of these; so if we're in view 5 and we get 2f+1 view change messages wanting to
    // change to view 30, then it's pretty clear that we're not actually in view 5.
}

bool
pbft::is_valid_viewchange_message(const pbft_msg& viewchange_message, const bzn_envelope& original_msg ) const
{
    if (!this->is_peer(original_msg.sender()))// TODO: Rich - If redundant, keep this check, remove the other
    {
        LOG(debug) << "is_valid_viewchange_message - message not from a peer";
        return false;
    }

    const auto valid_checkpoint = this->validate_viewchange_checkpoints(viewchange_message);
    if (valid_checkpoint == std::nullopt)
    {
        LOG(error) << "is_valid_viewchange_message - the checkpoint is invalid";
        return false;
    }

    bzn_envelope pre_prepare_envelope;
    // all the the prepared proofs are valid  (contains a pre prepare and 2f+1 mathcin prepares)
    for (int i{0} ; i < viewchange_message.prepared_proofs_size() ; ++i)
    {

        if (
                !pre_prepare_envelope.ParseFromString(viewchange_message.prepared_proofs(i).pre_prepare())
                || !this->is_peer(pre_prepare_envelope.sender())
                || !this->crypto->verify(pre_prepare_envelope))
        {
            LOG(error) << "is_valid_viewchange_message - a pre prepare message has a bad envelope";
            return false;
        }


        pbft_msg preprepare_message;
        if (
                !preprepare_message.ParseFromString(pre_prepare_envelope.pbft())
                || ( preprepare_message.sequence() <= valid_checkpoint->first ) )
        {
            LOG(error) << "is_valid_viewchange_message - a pre prepare message has an invalid sequence number";
            return false;
        }

        std::set<uuid_t> senders;
        bzn_envelope prepare_envelope;
        for ( int j{0} ; j < viewchange_message.prepared_proofs(i).prepare_size() ; ++j )
        {
            if (
                    !prepare_envelope.ParseFromString(viewchange_message.prepared_proofs(i).prepare(j))
                    || !this->is_peer(prepare_envelope.sender())
                    || !this->crypto->verify(prepare_envelope))
            {
                LOG(error) << "is_valid_viewchange_message - a prepare message has a bad envelope";
                return false;
            }

            // does the sequence number, view number and hash match those of the pre prepare
            if (
                    pbft_msg prepare_msg;
                    !prepare_msg.ParseFromString(prepare_envelope.pbft())
                    || preprepare_message.sequence() != prepare_msg.sequence()
                    || preprepare_message.view() != prepare_msg.view()
                    || preprepare_message.request_hash() != prepare_msg.request_hash() )
            {
                LOG(error) << "is_valid_viewchange_message - a prepare message is invalid";
                return false;
            }

            senders.insert(prepare_envelope.sender());
        }

        if (senders.size() < 2 * this->max_faulty_nodes() + 1 )
        {
            LOG(error) << "is_valid_viewchange_message - there were not enough valid viewchange messages";
            return false;
        }
    }
    return valid_checkpoint != std::nullopt;
}


bool
pbft::get_sequences_and_request_hashes_from_proofs(
        const pbft_msg& viewchange_msg
        , std::set<std::pair<uint64_t, std::string>>& sequence_request_pairs ) const
{
    for (int j{0} ; j < viewchange_msg.prepared_proofs_size() ; ++j )
    {
        // --- add the sequence/hash to a set
        auto prepared_proof = viewchange_msg.prepared_proofs(j);

        pbft_msg msg;
        if (
                bzn_envelope envelope;
                !envelope.ParseFromString(prepared_proof.pre_prepare())
                || !this->is_peer(envelope.sender())
                || !msg.ParseFromString(envelope.pbft()))
        {
            return false;
        }
        sequence_request_pairs.insert(std::make_pair(msg.sequence(), msg.request_hash() ));
    }
    return true;
}


bool
pbft::is_valid_newview_message(const pbft_msg& msg, const bzn_envelope& /*original_msg*/) const
{
    // - does it contain 2f+1 viewchange messages
    if (static_cast<size_t>(msg.viewchange_messages_size()) < 2 * this->max_faulty_nodes() + 1)
    {
        return false;
    }

    pbft_msg viewchange_msg;
    std::set<std::string> viewchange_senders;
    for (int i{0} ; i < msg.viewchange_messages_size() ; ++i)
    {

        bzn_envelope original_msg;
        // - are each of those viewchange messages valid?
        if (
                !original_msg.ParseFromString(viewchange_msg.viewchange_messages(i))
                || !viewchange_msg.ParseFromString(original_msg.pbft())
                || !this->is_valid_viewchange_message(viewchange_msg, original_msg)
                )

        {
            return false;
        }

        viewchange_senders.insert(original_msg.sender());
        std::set<std::pair<uint64_t, std::string>> sequence_request_pairs;

        if (!this->get_sequences_and_request_hashes_from_proofs(
                viewchange_msg
                , sequence_request_pairs))
        {
            return false;
        }

        // each *pair* must match a pre-prepare in the the newview message...
        std::vector<pbft_msg> matched_pre_prepares;
        for(const auto sequence_requesthash : sequence_request_pairs )
        {
            uint64_t sequence{sequence_requesthash.first};
            std::string request_hash{sequence_requesthash.second};

            for( int j{0} ; j <= msg.pre_prepare_messages_size() ; ++j )
            {
                const bzn_envelope& pre_prepare_envelope = msg.pre_prepare_messages(j);
                // TODO: do we need to cryptographically validate pre_prepare_envelope?
                // TODO: do we need to check the sender?

                pbft_msg pre_prepare;
                if (!pre_prepare.ParseFromString(pre_prepare_envelope.pbft()))
                {
                    LOG (error) << "is_valid_newview_message - pre_prepare not valid";
                    return false;
                }

                // did we find the sequence?
                if(
                        sequence == pre_prepare.sequence()
                        && request_hash == pre_prepare.request_hash())
                {
                    matched_pre_prepares.emplace_back(pre_prepare);
                    continue;
                }
            }
        }

        if( matched_pre_prepares.size() != size_t(msg.pre_prepare_messages_size()) )
        {
            LOG (error) << "is_valid_newview_message - unexpected sequence/request hash count";
            return false;
        }
    }

    if ( viewchange_senders.size() != size_t(msg.viewchange_messages_size()) )
    {
        LOG (error) << "is_valid_newview_message - unexpected viewchange message count";
        return false;
    }

    return true;
}

void
pbft::fill_in_missing_pre_prepares(std::map<uint64_t, bzn_envelope> &pre_prepares)
{
    uint64_t last_sequence_number = pre_prepares.empty() ? 0 : (pre_prepares.end()--)->first;
    for (uint64_t i = this->latest_stable_checkpoint().first + 1; i < last_sequence_number ; ++i )
    {
        //  -- create a new preprepare for a no-op operation using this sequence number
        if (pre_prepares.find(i) == pre_prepares.end())
        {
            auto msg = new database_msg;
            msg->set_allocated_nullmsg(new database_nullmsg);

            bzn_envelope request;
            request.set_database_msg(msg->SerializeAsString());
            this->crypto->sign(request);
            pre_prepares[i] = request;
        }
    }
}


/**
 * Create a <NEWVIEW, v+1, V, O> message
 * @param new_view_index v + 1
 * @param view_change_messages V
 * @param preprepares O
 * @return pbft_msg containing a new view message
 */
pbft_msg
pbft::make_newview(
        uint64_t new_view_index
        , const std::vector<pbft_msg> &view_change_messages
        , const std::map<uint64_t
        , bzn_envelope> &pre_prepare_messages
)
{
    pbft_msg newview;
    newview.set_type(PBFT_MSG_NEWVIEW);
    newview.set_view(new_view_index); //          v+1 is the new view index

    //          V is the set of 2f+1 view change messages
    for (const auto &viewchange : view_change_messages)
    {
        newview.add_viewchange_messages(viewchange.SerializeAsString());
    }

    // O
    for (const auto& preprepare_message : pre_prepare_messages)
    {
        *(newview.add_pre_prepare_messages()) = preprepare_message.second;
    }

    return newview;
}


pbft_msg
pbft::build_newview(uint64_t new_view, const std::vector<pbft_msg> &viewchange_messages)
{
    //  Computing O (set of new preprepares for new-view message)
    std::map<uint64_t, bzn_envelope> pre_prepares;

    //  - for each of the 2f+1 viewchange messages
    for (const auto& viewchange_message : viewchange_messages)
    {
        //  -- for each operation included in that viewchange message

        for (int i{0} ; i < viewchange_message.prepared_proofs_size() ; ++i )
        {
            const auto prepared_proof = viewchange_message.prepared_proofs(i);

            //  --- if we have not already created a new preprepare for an operation with that sequence number

            //  ---- Create a new preprepare for that operation using its original sequence number
            //       and request hash, but using the new view number

            bzn_envelope preprepare_envelope;
            preprepare_envelope.ParseFromString(prepared_proof.pre_prepare());

            pbft_msg pre_prepare;
            pre_prepare.ParseFromString(preprepare_envelope.pbft());
            pre_prepare.set_view(new_view);

            if (pre_prepare.sequence() < this->latest_stable_checkpoint().first)
            {
                continue;
            }
            pre_prepares[pre_prepare.sequence()] = this->make_signed_envelope(pre_prepare.SerializeAsString());
        }

        //  - add each preprepare created this way to the new-view message
    }

    //  - for each sequence number between the base-sequence number (the last stable checkpoint)
    //    and the highest sequence number for which a new preprepare was created

    // Fill in missing messages with no ops
    this->fill_in_missing_pre_prepares(pre_prepares);

    //  - add each preprepare created this way to the new-view message
    // std::set<std::string> view_change_messages
    // viewchange_messages

    return this->make_newview(new_view, viewchange_messages, pre_prepares);
}


bzn_envelope
pbft::make_signed_envelope(std::string serialized_pbft_message)
{
    bzn_envelope envelope;
    envelope.set_pbft(serialized_pbft_message);
    envelope.set_sender(this->get_uuid());
    this->crypto->sign(envelope);
    return envelope;
}


/**
 *
 * @param msg
 */
void
pbft::save_checkpoint(const pbft_msg& msg)
{
    pbft_msg message;
    for (int i{0} ; i < msg.checkpoint_messages_size() ; ++i )
    {
        const auto checkpoint_msg = msg.checkpoint_messages(i);
        bzn_envelope original_checkpoint;
        if (
                original_checkpoint.ParseFromString(checkpoint_msg)
                || !this->crypto->verify(original_checkpoint) // TODO: consider removing this - check with debugger
                || !message.ParseFromString(original_checkpoint.pbft())
           )
        {
            continue;
        }
        this->handle_checkpoint(message, original_checkpoint);
    }
}


void
pbft::handle_viewchange(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    LOG(debug) << "Handle_viewchange: " << msg.SerializeAsString()  << " -- " << original_msg.SerializeAsString();

    if (this->is_valid_viewchange_message(msg, original_msg))
    {
        LOG(debug) << "handle_viewchange - adding valid viewchange message";
        this->valid_view_change_messages[msg.view()].insert(original_msg.SerializeAsString());
    }
    else
    {
        LOG(error) << "handle_viewchange - invalid viewchange message, ignoring";
        return;
    }

    this->save_checkpoint(msg);

// this needs to be done for ALL replicas....
    // When a replica recieves f + 1 view change messages it sends one as well
    // should it be == or >=  ?
    if (
            this->view_is_valid
            && this->valid_view_change_messages.size() == this->max_faulty_nodes() + 1)
    {
        // replica sends VIEWCHANGE message
        LOG (debug) << "handle_viewchange - replica is broadcasting viewchange message";
        pbft_msg viewchange_message{
                make_viewchange(
                        msg.view()
                        , this->latest_stable_checkpoint().first
                        , this->stable_checkpoint_proof
                        , this->prepared_operations_since_last_checkpoint())
        };



        this->view_is_valid = false;
        this->broadcast(this->wrap_message(viewchange_message));
    }





    // TODO move this to the end...
    // we want to filter std::map<uint64_t, std::set<bzn_envelope>> valid_view_change_messages for
    // 1) we would be the primary for that view
    // 2) that view is greater than the current view
    // 3) we have 2 f + 1 viewchange messages for it
    const auto viewchange = std::find_if(
            this->valid_view_change_messages.begin()
            , this->valid_view_change_messages.end()
            , [&](const auto& p)
            {
                return ( (this->get_primary(p.first).uuid == this->get_uuid())
                    &&  (p.first > this->view)
                    && (p.second.size() >= 2 * this->max_faulty_nodes() + 1) );
            });

    if (viewchange == this->valid_view_change_messages.end())
    {
        LOG (error) << "handle_viewchange - no valid viewchange messages";
        return;
    }



    // When the primary of the new view recieves 2f valid view change messages for the new view
    // the primary of new view broadcasts NEWVIEW
    if (
            this->valid_view_change_messages.size() == 2 * this->max_faulty_nodes()
            && this->get_primary(msg.view()).uuid == this->get_uuid() )
    {
        // create the newview and and broadcast it

        // viewchange->second is a set of bzn_envelope strings
        // -- we want a of set<pbft_msg>

        std::vector<pbft_msg> viewchange_messages;
        for (const auto& view_change_msg : viewchange->second)
        {
            pbft_msg pbft_viewchange;
            if (!pbft_viewchange.ParseFromString(view_change_msg))
            {
                LOG (error) << "handle_viewchange - unable to parse viewchange message";
                continue;
            }

            // Since sets require hashable objects we needed to use vectors and
            // do our own uniqueness testing.
            // TODO: use a set with a hash function
            auto found_iter = std::find_if(
                    std::cbegin(viewchange_messages)
                    , std::cend(viewchange_messages)
                    , [&](const auto& viewchange)
                        {
                            return !google::protobuf::util::MessageDifferencer::Equals(viewchange, pbft_viewchange);
                        }
                    );
            if ( found_iter == viewchange_messages.end())
            {
                viewchange_messages.emplace_back(pbft_viewchange);
            }
        }

        auto new_view = this->build_newview(viewchange->first, viewchange_messages);
        this->broadcast(this->wrap_message(new_view));

        // primary of the new view moves to new view
        this->view = msg.view();
    }
}

void
pbft::handle_newview(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    if (!this->is_valid_newview_message(msg, original_msg))
    {
        LOG (debug) << "handle_newview - ignoring invalid NEWVIEW message ";
        return;
    }

    LOG(debug) << "handle_newview - recieved valid NEWVIEW message";

    this->view = msg.view();
    this->view_is_valid = true;

    // after moving to the new view processes the preprepares
    for (size_t i{0} ; i < static_cast<size_t>(msg.pre_prepare_messages_size()) ; ++i)
    {
        const bzn_envelope original_msg = msg.pre_prepare_messages(i);
        pbft_msg msg;
        assert(msg.ParseFromString(original_msg.pbft()));
        this->handle_preprepare( msg, original_msg);
    }
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

    status["latest_stable_checkpoint"]["sequence_number"] = this->latest_stable_checkpoint().first;
    status["latest_stable_checkpoint"]["hash"] = this->latest_stable_checkpoint().second;
    status["latest_checkpoint"]["sequence_number"] = this->latest_checkpoint().first;
    status["latest_checkpoint"]["hash"] = this->latest_checkpoint().second;

    status["unstable_checkpoints_count"] = uint64_t(this->unstable_checkpoints_count());
    status["next_issued_sequence_number"] = this->next_issued_sequence_number;
    status["view"] = this->view;

    status["peer_index"] = bzn::json_message();
    for (const auto& p : this->current_peers())
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

bool
pbft::initialize_configuration(const bzn::peers_list_t& peers)
{
    auto config = std::make_shared<pbft_configuration>();
    bool config_good = true;
    for (auto& p : peers)
    {
        config_good &= config->add_peer(p);
    }

    if (!config_good)
    {
        LOG(warning) << "One or more peers could not be added to configuration";
    }

    this->configurations.add(config);
    this->configurations.enable(config->get_hash());
    this->configurations.set_current(config->get_hash());

    return config_good;
}

std::shared_ptr<const std::vector<bzn::peer_address_t>>
pbft::current_peers_ptr() const
{
    auto config = this->configurations.current();
    if (config)
    {
        return config->get_peers();
    }

    throw std::runtime_error("No current configuration!");
}

const std::vector<bzn::peer_address_t>&
pbft::current_peers() const
{
    return *(this->current_peers_ptr());
}

const peer_address_t&
pbft::get_peer_by_uuid(const std::string& uuid) const
{
    for (auto const& peer : this->current_peers())
    {
        if (peer.uuid == uuid)
        {
            return peer;
        }
    }

    // something went wrong. this uuid should exist
    throw std::runtime_error("peer missing from peers list");
}

void
pbft::broadcast_new_configuration(pbft_configuration::shared_const_ptr config)
{
    auto cfg_msg = new pbft_config_msg;
    cfg_msg->set_configuration(config->to_string());
    bzn_envelope req;
    req.set_pbft_internal_request(cfg_msg->SerializeAsString());

    auto op = this->setup_request_operation(req, this->crypto->hash(req));
    this->do_preprepare(op);
}

bool
pbft::is_configuration_acceptable_in_new_view(hash_t config_hash)
{
    return this->configurations.is_enabled(config_hash);
}

void
pbft::handle_config_message(const pbft_msg& msg, const std::shared_ptr<pbft_operation>& op)
{
    assert(op->has_config_request());
    auto config = std::make_shared<pbft_configuration>();
    if (msg.type() == PBFT_MSG_PREPREPARE && config->from_string(op->get_config_request().configuration()))
    {
        if (this->proposed_config_is_acceptable(config))
        {
            // store this configuration
            this->configurations.add(config);
        }
    }
}

bool
pbft::move_to_new_configuration(hash_t config_hash)
{
    if (this->configurations.is_enabled(config_hash))
    {
        this->configurations.set_current(config_hash);
        this->configurations.remove_prior_to(config_hash);
        return true;
    }

    return false;
}

bool
pbft::proposed_config_is_acceptable(std::shared_ptr<pbft_configuration> /* config */)
{
    return true;
}

std::set<std::shared_ptr<bzn::pbft_operation>>
pbft::prepared_operations_since_last_checkpoint()
{
    std::set<std::shared_ptr<bzn::pbft_operation>> retval;
    for (const auto& p : this->operations)
    {
        if ( p.second->is_prepared() )
        {
            if (p.second->sequence > this->latest_stable_checkpoint().first)
            {
                retval.emplace(p.second);
            }
        }
    }
    return retval;
}

timestamp_t
pbft::now() const
{
    return static_cast<timestamp_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

void
pbft::saw_request(const bzn_envelope& req, const request_hash_t& hash)
{
    this->recent_requests.insert(std::make_pair(req.timestamp(),
        std::make_pair(req.sender(), hash)));
}

bool
pbft::already_seen_request(const bzn_envelope& req, const request_hash_t& hash) const
{
    auto range = this->recent_requests.equal_range(req.timestamp());
    for (auto r = range.first; r != range.second; r++)
    {
        if (r->second.first == req.sender() && r->second.second == hash)
        {
            return true;
        }
    }

    return false;
}

/*!
 * Creates <VIEW-CHANGE v+1, n, C, P, i>_sigma_i
 * @param new_view new view (v+1)
 * @param base_sequence_number sequence of last stable checkpoint (n)
 * @param stable_checkpoint_proof proof of checkpoint C
 * @param prepared_operations set of messages after i was prepared
 * @param i the uuid of the sender
 *
 * @return <VIEW-CHANGE v+1, n, C, P, i>_sigma_i
 */
pbft_msg
pbft::make_viewchange(
        uint64_t new_view
        , uint64_t base_sequence_number
        , std::unordered_map<bzn::uuid_t, std::string> stable_checkpoint_proof
        , std::set<std::shared_ptr<bzn::pbft_operation>> prepared_operations)
{
    pbft_msg viewchange;

    viewchange.set_type(PBFT_MSG_VIEWCHANGE);
    viewchange.set_view(new_view);
    viewchange.set_sequence(base_sequence_number);  // base_sequence_number = n = sequence # of last valid checkpoint

    // C = a set of local 2*f + 1 valid checkpoint messages
    for (const auto& msg : stable_checkpoint_proof)
    {
        viewchange.add_checkpoint_messages(msg.second);  //C
    }

    // P = a set (of client requests) containing a set P_m  for each request m that prepared at i with a sequence # higher than n
    // P_m = the pre prepare and the 2 f + 1 prepares
    //            get the set of operations, frome each operation get the messages..
    for (const auto& operation : prepared_operations)
    {
        auto prepared_proofs = viewchange.add_prepared_proofs();
        prepared_proofs->set_pre_prepare(operation->get_preprepare());
        for (const auto &prepared : operation->get_prepares())
        {
            prepared_proofs->add_prepare(prepared);
        }
    }

    return viewchange;
}
