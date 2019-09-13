// Copyrigh (C) 2018 Bluzelle 
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
#include <pbft/operations/pbft_memory_operation.hpp>
#include <pbft/operations/pbft_operation_manager.hpp>
#include <utils/make_endpoint.hpp>
#include <utils/bytes_to_debug_string.hpp>
#include <google/protobuf/text_format.h>
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
    , std::shared_ptr<bzn::peers_beacon_base> peers
    , std::shared_ptr<bzn::options_base> options
    , std::shared_ptr<pbft_service_base> service
    , std::shared_ptr<pbft_failure_detector_base> failure_detector
    , std::shared_ptr<bzn::crypto_base> crypto
    , std::shared_ptr<bzn::pbft_operation_manager> operation_manager
    , std::shared_ptr<bzn::storage_base> storage
    , std::shared_ptr<bzn::monitor_base> monitor
    )
    : storage(storage)
    , node(std::move(node))
    , uuid(options->get_uuid())
    , options(options)
    , service(std::move(service))
    , failure_detector(std::move(failure_detector))
    , io_context(io_context)
    , audit_heartbeat_timer(this->io_context->make_unique_steady_timer())
    , crypto(std::move(crypto))
    , operation_manager(std::move(operation_manager))
    , peers_beacon(std::move(peers))
    , checkpoint_manager(std::make_shared<pbft_checkpoint_manager>(this->io_context, this->storage, this->peers_beacon, this->node))
    , monitor(std::move(monitor))
{
    if (this->peers_beacon->current()->empty())
    {
        throw std::runtime_error("No peers found!");
    }

    this->initialize_persistent_state();

    this->service->save_service_state_at(((this->next_issued_sequence_number.value() / CHECKPOINT_INTERVAL) + 1) * CHECKPOINT_INTERVAL);
}

void
pbft::start()
{
    std::call_once(this->start_once,
        [this]()
        {
            this->node->register_for_message(bzn_envelope::kPbft,
                std::bind(&pbft::handle_bzn_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

            this->node->register_for_message(bzn_envelope::kPbftMembership,
                std::bind(&pbft::handle_membership_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

            this->node->register_for_message(bzn_envelope::kDatabaseMsg,
                std::bind(&pbft::handle_database_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

            this->node->register_for_message(bzn_envelope::kDatabaseResponse,
                std::bind(&pbft::handle_database_response_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

            this->checkpoint_manager->start();

            this->audit_heartbeat_timer->expires_from_now(HEARTBEAT_INTERVAL);
            this->audit_heartbeat_timer->async_wait(
                std::bind(&pbft::handle_audit_heartbeat_timeout, shared_from_this(), std::placeholders::_1));

            this->service->register_execute_handler(
                [weak_this = this->weak_from_this(), fd = this->failure_detector]
                (std::shared_ptr<pbft_operation> op)
                {
                    fd->request_executed(op->get_request_hash());

                    auto strong_this = weak_this.lock();
                    if (strong_this)
                    {
                        strong_this->last_executed_sequence_number = op->get_sequence();
                        if (op->get_sequence() % CHECKPOINT_INTERVAL == 0)
                        {
                            // tell service to save the next checkpoint after this one
                            strong_this->service->save_service_state_at(op->get_sequence() + CHECKPOINT_INTERVAL);

                            checkpoint_t chk{op->get_sequence(), strong_this->service->service_state_hash(
                                op->get_sequence())};
                            strong_this->checkpoint_manager->local_checkpoint_reached(chk);

                            const auto safe_delete_bound = std::min(
                                strong_this->checkpoint_manager->get_latest_stable_checkpoint().first
                                , strong_this->checkpoint_manager->get_latest_local_checkpoint().first);

                            strong_this->operation_manager->delete_operations_until(safe_delete_bound);
                        }
                    }
                    else
                    {
                        throw std::runtime_error("pbft_service callback failed because pbft does not exist");
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
        msg.mutable_primary_status()->set_view(this->view.value());
        msg.mutable_primary_status()->set_primary(this->uuid);

        this->async_signed_broadcast(msg);
    }

    this->audit_heartbeat_timer->expires_from_now(HEARTBEAT_INTERVAL);
    this->audit_heartbeat_timer->async_wait(std::bind(&pbft::handle_audit_heartbeat_timeout, shared_from_this(), std::placeholders::_1));
}

void
pbft::handle_bzn_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> /*session*/)
{
    if (msg.payload_case() != bzn_envelope::kPbft)
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
    LOG(debug) << "Received membership message: " << msg.ShortDebugString().substr(0, MAX_MESSAGE_SIZE);

    pbft_membership_msg inner_msg;
    if (!inner_msg.ParseFromString(msg.pbft_membership()))
    {
        LOG(error) << "Failed to parse payload of wrapped message " << msg.DebugString().substr(0, MAX_MESSAGE_SIZE);
        return;
    }

    const auto hash = this->crypto->hash(msg);

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    switch (inner_msg.type())
    {
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
    LOG(debug) << "Received message: " << msg.ShortDebugString().substr(0, MAX_MESSAGE_SIZE) << "\nFrom: " << original_msg.sender();

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    if (!this->preliminary_filter_msg(msg))
    {
        return;
    }

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
    if (!this->is_view_valid() && !(msg.type() == PBFT_MSG_VIEWCHANGE || msg.type() == PBFT_MSG_NEWVIEW))
    {
        LOG(debug) << "Dropping message because local view is invalid";
        return false;
    }

    if (auto t = msg.type();t == PBFT_MSG_PREPREPARE || t == PBFT_MSG_PREPARE || t == PBFT_MSG_COMMIT)
    {
        if (msg.view() != this->view.value())
        {
            LOG(debug) << "Dropping message because it has the wrong view number";
            return false;
        }

        if (msg.sequence() <= this->get_low_water_mark())
        {
            LOG(debug) << boost::format("Dropping message because sequence number %1% <= %2%") % msg.sequence()
                % this->get_low_water_mark();
            return false;
        }

        if (msg.sequence() > this->get_high_water_mark())
        {
            LOG(debug) << boost::format("Dropping message because sequence number %1% greater than %2%") % msg.sequence()
                % this->get_high_water_mark();
            return false;
        }
    }

    return true;
}

std::shared_ptr<pbft_operation>
pbft::setup_request_operation(const bzn_envelope& request_env, const bzn::hash_t& request_hash)
{
    const uint64_t request_seq = this->next_issued_sequence_number.value();
    this->next_issued_sequence_number = request_seq + 1;
    auto op = this->operation_manager->find_or_construct(this->view.value(), request_seq, request_hash);
    op->record_request(request_env);

    return op;
}

void
pbft::send_error_response(const bzn_envelope& request_env, const std::shared_ptr<session_base>& session
    , const std::string& msg) const
{
    database_msg req;
    if (session && req.ParseFromString(request_env.database_msg()))
    {
        database_response resp;
        *resp.mutable_header() = req.header();
        resp.mutable_error()->set_message(msg);

        session->send_message(std::make_shared<std::string>(this->wrap_message(resp).SerializeAsString()));
    }
}

void
pbft::handle_request(const bzn_envelope& request_env, const std::shared_ptr<session_base>& session)
{
    if (request_env.timestamp() < (this->now() - MAX_REQUEST_AGE_MS) || request_env.timestamp() > (this->now() + MAX_REQUEST_AGE_MS))
    {
        this->send_error_response(request_env, session, TIMESTAMP_ERROR_MSG);

        LOG(info) << "Rejecting request because it is outside allowable timestamp range: "
                << request_env.ShortDebugString();
        return;
    }

    // Allowing the maximum size to simply be bzn::MAX_VALUE_SIZE does not take into account the overhead that the
    // primary peer will add to the message when it re-broadcasts the request to the other peers in the swarm. This
    // additional overhead has been experimentally determined, on MacOS, to be 245 bytes. For now we shall use the magic
    // number overhead size of 512 when we limit
    const size_t OVERHEAD_SIZE{512};
    if (!request_env.database_msg().empty() && request_env.database_msg().size() >= (bzn::MAX_VALUE_SIZE - OVERHEAD_SIZE))
    {
        this->send_error_response(request_env, session, TOO_LARGE_ERROR_MSG);

        LOG(warning) << "Rejecting request because it is too large [" << request_env.database_msg().size() << " bytes]";
        return;
    }

    const auto hash = this->crypto->hash(request_env);

    if (session)
    {
        if (this->sessions_waiting_on_forwarded_requests.find(hash) == this->sessions_waiting_on_forwarded_requests.end())
        {
            this->add_session_to_sessions_waiting(hash, session);
        }
    }

    this->monitor->start_timer(hash);

    if (!this->is_primary())
    {
        this->forward_request_to_primary(request_env);
        return;
    }

    // keep track of what requests we've seen based on timestamp and only send preprepares once
    if (this->already_seen_request(request_env, hash))
    {
        // TODO: send error message to client
        LOG(debug) << "Rejecting duplicate request: " << request_env.ShortDebugString().substr(0, MAX_MESSAGE_SIZE);
        return;
    }
    this->saw_request(request_env, hash);
    auto op = setup_request_operation(request_env, hash);
    this->do_preprepare(op);
}

void
pbft::forward_request_to_primary(const bzn_envelope& request_env)
{
    this->node
        ->send_signed_message(bzn::make_endpoint(this->get_primary()), std::make_shared<bzn_envelope>(request_env));

    const bzn::hash_t req_hash = this->crypto->hash(request_env);
    LOG(info) << "Forwarded request to primary, " << bzn::bytes_to_debug_string(req_hash);

    this->failure_detector->request_seen(req_hash);
}


void
pbft::maybe_record_request(const bzn_envelope &request_env, const std::shared_ptr<pbft_operation> &op)
{
    if (request_env.payload_case() == bzn_envelope::PayloadCase::PAYLOAD_NOT_SET || this->crypto->hash(request_env) != op->get_request_hash())
    {
        LOG(trace) << "Not recording request because hashes do not match";
        return;
    }
    op->record_request(request_env);
}


void
pbft::handle_preprepare(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    // If we've already accepted a preprepare for this view+sequence, and it's not this one, then we should reject this one
    // Note that if we get the same preprepare more than once, we can still accept it
    const log_key_t log_key(msg.view(), msg.sequence());

    if (auto lookup = this->accepted_preprepares.find(log_key);
        lookup != this->accepted_preprepares.end()
        && std::get<2>(lookup->second.value()) != msg.request_hash())
    {
        LOG(debug) << "Rejecting preprepare: already accepted a conflicting one";
        return;
    }
    else
    {
        auto op = this->operation_manager->find_or_construct(msg);
        auto env_copy{original_msg};
        env_copy.clear_piggybacked_requests();
        op->record_pbft_msg(msg, env_copy);

        this->maybe_record_request(msg.request(), op);
        if (original_msg.piggybacked_requests_size())
        {
            this->maybe_record_request(original_msg.piggybacked_requests(0), op);
        }

        // This assignment will be redundant if we've seen this preprepare before, but that's fine
        accepted_preprepares[log_key] = persistent<bzn::operation_key_t>{this->storage, op->get_operation_key()
            , ACCEPTED_PREPREPARES_KEY, log_key};


        this->do_preprepared(op);
        this->maybe_advance_operation_state(op);
    }
}

void
pbft::handle_prepare(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    // Prepare messages are never rejected, assuming the sanity checks passed
    auto op = this->operation_manager->find_or_construct(msg);

    op->record_pbft_msg(msg, original_msg);
    this->maybe_advance_operation_state(op);
}

void
pbft::handle_commit(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    // Commit messages are never rejected, assuming  the sanity checks passed
    auto op = this->operation_manager->find_or_construct(msg);

    op->record_pbft_msg(msg, original_msg);
    this->maybe_advance_operation_state(op);
}

void
pbft::handle_get_state(const pbft_membership_msg& msg, std::shared_ptr<bzn::session_base> session) const
{
    LOG(debug) << boost::format("Got request for state data for checkpoint: seq: %1%, hash: %2%")
                  % msg.sequence() % msg.state_hash();

    // get stable checkpoint for request
    checkpoint_t req_cp(msg.sequence(), msg.state_hash());
    std::shared_ptr<std::string> state;
    if (this->checkpoint_manager->get_latest_stable_checkpoint() != req_cp || !(state = this->get_checkpoint_state(req_cp)))
    {
        LOG(debug) << boost::format("I'm missing data for checkpoint: seq: %1%, hash: %2%")
                      % msg.sequence() % msg.state_hash();
        // TODO: send error response
        return;
    }

    if (this->get_view() > 1 && this->saved_newview.payload_case() != bzn_envelope::kPbft)
    {
        LOG(warning) << "No saved NEWVIEW message to send for state message. Not responding";
        return;
    }

    pbft_membership_msg reply;
    reply.set_type(PBFT_MMSG_SET_STATE);
    reply.set_sequence(req_cp.first);
    reply.set_state_hash(req_cp.second);
    reply.set_state_data(*state);

    // TODO: the latest stable checkpoint may have advanced by the time we pull it here, which will cause the receiver
    // to reject this message. This is innocuous, but we could avoid the awkwardness by requesting a specific checkpoint
    // proof from the manager instead of the latest.
    for (const auto& pair : this->checkpoint_manager->get_latest_stable_checkpoint_proof())
    {
        bzn_envelope checkpoint_claim;
        checkpoint_claim.ParseFromString(pair.second);
        *(reply.add_checkpoint_proof()) = checkpoint_claim;
    }

    if (this->saved_newview.payload_case() == bzn_envelope::kPbft)
    {
        reply.set_allocated_newview_msg(new bzn_envelope(this->saved_newview));
    }

    auto msg_ptr = std::make_shared<bzn::encoded_message>(this->wrap_message(reply).SerializeAsString());
    session->send_message(msg_ptr);
}

void
pbft::handle_set_state(const pbft_membership_msg& msg)
{
    checkpoint_t cp(msg.sequence(), msg.state_hash());

    for (size_t i{0}; i < static_cast<uint64_t>(msg.checkpoint_proof_size()); i++)
    {
        this->checkpoint_manager->handle_checkpoint_message(msg.checkpoint_proof(i));
    }

    if (this->checkpoint_manager->get_latest_stable_checkpoint() != cp)
    {
        LOG(info) << boost::format("Ignoring state message at %1% because it does not match latest stable checkpoint") % msg.sequence();
        return;
    }

    if (this->checkpoint_manager->get_latest_local_checkpoint() == cp)
    {
        LOG(info) << boost::format("Ignoring state message at %1% because we already have that state") % msg.sequence();
        return;
    }

    if (this->checkpoint_manager->get_latest_local_checkpoint().first > msg.sequence())
    {
        LOG(info) << boost::format("Ignoring state message at %1% because we have a newer state") % msg.sequence();
        return;
    }

    LOG(info) << boost::format("Adopting checkpoint %1% at seq %2%")
        % cp.second % cp.first;

    // TODO: validate the state data
    this->set_checkpoint_state(cp, msg.state_data());

    // TODO: This should maybe use normal newview handling
    if (msg.has_newview_msg())
    {
        pbft_msg newview;
        if (newview.ParseFromString(msg.newview_msg().pbft()))
        {
            this->view = newview.view();
            LOG(info) << "setting view to " << this->view.value();
        }
    }
}

void
pbft::broadcast(const bzn_envelope& msg)
{
    auto msg_ptr = std::make_shared<bzn_envelope>(msg);

    for (const auto& peer : *this->peers_beacon->current())
    {
        this->node->send_signed_message(make_endpoint(peer), msg_ptr);
    }
}

void
pbft::async_signed_broadcast(const pbft_membership_msg& msg)
{
    auto msg_env = std::make_shared<bzn_envelope>();
    msg_env->set_pbft_membership(msg.SerializeAsString());
    this->async_signed_broadcast(std::move(msg_env));
}

void
pbft::async_signed_broadcast(const pbft_msg& msg)
{
    auto msg_env = std::make_shared<bzn_envelope>();
    msg_env->set_pbft(msg.SerializeAsString());
    this->async_signed_broadcast(std::move(msg_env));
}

void
pbft::async_signed_broadcast(const audit_message& msg)
{
    auto msg_env = std::make_shared<bzn_envelope>();
    msg_env->set_audit(msg.SerializeAsString());
    this->async_signed_broadcast(std::move(msg_env));
}

void
pbft::async_signed_broadcast(std::shared_ptr<bzn_envelope> msg_env)
{
    msg_env->set_timestamp(this->now());

    auto targets = std::make_shared<std::vector<boost::asio::ip::tcp::endpoint>>();
    for (const auto& peer : *this->peers_beacon->current())
    {
        targets->emplace_back(bzn::make_endpoint(peer));
    }

    this->node->multicast_signed_message(std::move(targets), msg_env);
}

void
pbft::maybe_advance_operation_state(const std::shared_ptr<pbft_operation>& op)
{
    if (op->get_stage() == pbft_operation_stage::prepare && op->is_ready_for_commit())
    {
        this->do_prepared(op);
    }

    if (op->get_stage() == pbft_operation_stage::commit && op->is_ready_for_execute())
    {
        this->do_committed(op);
    }
}

pbft_msg
pbft::common_message_setup(const std::shared_ptr<pbft_operation>& op, pbft_msg_type type)
{
    pbft_msg msg;
    msg.set_view(op->get_view());
    msg.set_sequence(op->get_sequence());
    msg.set_request_hash(op->get_request_hash());
    msg.set_type(type);

    return msg;
}

void
pbft::do_preprepare(const std::shared_ptr<pbft_operation>& op)
{
    LOG(debug) << "Doing preprepare for operation " << op->get_sequence();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPREPARE);

    const bzn_envelope& req_env = op->get_request();
    auto msg_env = std::make_shared<bzn_envelope>();
    if (req_env.payload_case() == bzn_envelope::kPbftInternalRequest)
    {
        msg.set_allocated_request(new bzn_envelope(req_env));
        msg.set_request_type(pbft_request_type::PBFT_REQUEST_INTERNAL);
    }
    else
    {
        *(msg_env->add_piggybacked_requests()) = req_env;
    }

    msg_env->set_pbft(msg.SerializeAsString());
    this->async_signed_broadcast(std::move(msg_env));
}

void
pbft::do_preprepared(const std::shared_ptr<pbft_operation>& op)
{
    LOG(debug) << "Entering prepare phase for operation " << op->get_sequence();

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_PREPARE);

    this->async_signed_broadcast(msg);
}

void
pbft::do_prepared(const std::shared_ptr<pbft_operation>& op)
{
    LOG(debug) << "Entering commit phase for operation " << op->get_sequence();
    op->advance_operation_stage(pbft_operation_stage::commit);

    pbft_msg msg = this->common_message_setup(op, PBFT_MSG_COMMIT);

    this->async_signed_broadcast(msg);
}

void
pbft::do_committed(const std::shared_ptr<pbft_operation>& op)
{
    LOG(debug) << "Operation " << op->get_sequence() << " " << bzn::bytes_to_debug_string(op->get_request_hash()) << " is committed-local";
    op->advance_operation_stage(pbft_operation_stage::execute);

    // If we have a pending session for this request, attach to the operation just before we pass off to the service.
    // If we were to do that before this moment, then maybe the pbft_operation we attached it to never gets executed and
    // it gets executed in a different view (earlier or later), so the client never gets a response. If we do it here,
    // then the only reason this pbft_operation instance wouldn't be executed eventually is if the service already has
    // a pbft_operation for this sequence, meaning we committed this before we got the request, or if we crash, which
    // kills the session anyway.

    const auto session = this->sessions_waiting_on_forwarded_requests.find(op->get_request_hash());
    if (session != this->sessions_waiting_on_forwarded_requests.end() && !op->has_session())
    {
        LOG(debug) << "Found pending session for this operation";
        op->set_session(session->second);
    }
    else
    {
        LOG(debug) << "No pending session for this operation";
    }

    if (this->audit_enabled)
    {
        audit_message msg;
        msg.mutable_pbft_commit()->set_operation(op->get_request_hash());
        msg.mutable_pbft_commit()->set_sequence_number(op->get_sequence());
        msg.mutable_pbft_commit()->set_sender_uuid(this->uuid);

        this->async_signed_broadcast(msg);
    }

    // TODO: this needs to be refactored to be service-agnostic
    if (op->has_db_request())
    {
        LOG(debug) << "Posting operation " << op->get_sequence() << " for execution";
        this->io_context->post(std::bind(&pbft_service_base::apply_operation, this->service, op));
    }
    else
    {
        LOG(debug) << "No db_request for operation " << op->get_sequence();

        // the service needs sequentially sequenced operations. post a null request to fill in this hole
        database_msg msg;
        msg.mutable_nullmsg();
        bzn_envelope request;
        request.set_database_msg(msg.SerializeAsString());
        auto new_op = std::make_shared<pbft_memory_operation>(op->get_view(), op->get_sequence()
            , this->crypto->hash(request), nullptr);
        new_op->record_request(request);
        this->io_context->post(std::bind(&pbft_service_base::apply_operation, this->service, new_op));
    }
}

bool
pbft::is_primary() const
{
    return this->get_primary().uuid == this->uuid;
}

peer_address_t
pbft::get_primary(std::optional<uint64_t> view) const
{
    return this->peers_beacon->ordered()->at(view.value_or(this->view.value()) % this->peers_beacon->current()->size());
}

bzn_envelope
pbft::wrap_message(bzn_envelope& env) const
{
    env.set_sender(this->uuid);
    env.set_timestamp(this->now());
    env.set_swarm_id(this->options->get_swarm_id());
    this->crypto->sign(env);

    return env;
}

bzn_envelope
pbft::wrap_message(const pbft_msg& msg) const
{
    bzn_envelope result;
    result.set_pbft(msg.SerializeAsString());
    return this->wrap_message(result);
}

bzn_envelope
pbft::wrap_message(const pbft_membership_msg& msg) const
{
    bzn_envelope result;
    result.set_pbft_membership(msg.SerializeAsString());
    return this->wrap_message(result);
}

bzn_envelope
pbft::wrap_message(const database_response& msg) const
{
    bzn_envelope result;
    result.set_database_response(msg.SerializeAsString());
    return this->wrap_message(result);
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
        this->async_signed_broadcast(msg);
    }
}

void
pbft::handle_failure()
{
    std::lock_guard<std::mutex> lock(this->pbft_lock);
    LOG (error) << "handle_failure - PBFT failure - invalidating current view and sending VIEWCHANGE to view: "
        << this->view.value() + 1;
    this->notify_audit_failure_detected();
    this->initiate_viewchange();
}

void
pbft::initiate_viewchange()
{
    pbft_msg view_change;
    std::vector<bzn_envelope> requests;

    this->view_is_valid = false;
    auto msg_env = pbft::make_viewchange(this->view.value() + 1
            , this->checkpoint_manager->get_latest_stable_checkpoint().first
            , this->checkpoint_manager->get_latest_stable_checkpoint_proof()
            , this->operation_manager->prepared_operations_since(this->checkpoint_manager->get_latest_stable_checkpoint().first));

    LOG(debug) << "Sending VIEWCHANGE for view " << this->view.value() + 1;

    this->async_signed_broadcast(std::move(msg_env));
}

std::shared_ptr<std::string>
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

size_t
pbft::quorum_size() const
{
    return 1 + (2*this->max_faulty_nodes());
}

size_t
pbft::max_faulty_nodes() const
{
    return this->peers_beacon->current()->size()/3;
}

void
pbft::handle_database_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session)
{
    LOG(debug) << "got database message";

    if (!this->service->apply_operation_now(msg, session))
    {
        std::lock_guard<std::mutex> lock(this->pbft_lock);

        if (msg.timestamp() == 0)
        {
            bzn_envelope mutable_msg(msg);
            mutable_msg.set_timestamp(this->now());
            this->handle_request(mutable_msg, session);
        }
        else
        {
            this->handle_request(msg, session);
        }
    }
}


void
pbft::handle_database_response_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> /*session*/)
{
    database_msg db_msg;

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    if (db_msg.ParseFromString(msg.database_response()))
    {
        if (const auto session_it = this->sessions_waiting_on_forwarded_requests.find(db_msg.header().request_hash());
            session_it != this->sessions_waiting_on_forwarded_requests.end())
        {
            session_it->second->send_message(std::make_shared<bzn::encoded_message>(msg.SerializeAsString()));
            this->monitor->finish_timer(bzn::statistic::request_latency, db_msg.header().request_hash());
            return;
        }

        LOG(warning) << "session not found for database response";
        return;
    }

    LOG(error) << "failed to read database response";
}

uint64_t
pbft::get_low_water_mark()
{
    return this->checkpoint_manager->get_latest_stable_checkpoint().first;
}

uint64_t
pbft::get_high_water_mark()
{
    return this->checkpoint_manager->get_latest_stable_checkpoint().first + HIGH_WATER_INTERVAL_IN_CHECKPOINTS * CHECKPOINT_INTERVAL;
}

bool
pbft::is_view_valid() const
{
    return this->view_is_valid.value();
}

uint64_t
pbft::get_view() const
{
    return this->view.value();
}

std::shared_ptr<bzn::node_base>
pbft::get_node()
{
    return this->node;
}

bool
pbft::is_peer(const bzn::uuid_t& sender) const
{
    auto ptr = this->peers_beacon->current();
    return std::find_if (ptr->begin(), ptr->end(), [&](const auto& address)
    {
        return sender == address.uuid;
    }) != ptr->end();
}

std::map<bzn::checkpoint_t, std::set<bzn::uuid_t>>
pbft::validate_and_extract_checkpoint_hashes(const pbft_msg &viewchange_message) const
{
    std::map<bzn::checkpoint_t , std::set<bzn::uuid_t>> checkpoint_hashes;
    for (size_t i{0}; i < static_cast<uint64_t>(viewchange_message.checkpoint_messages_size()); ++i)
    {
        const bzn_envelope& envelope{viewchange_message.checkpoint_messages(i)};
        checkpoint_msg checkpoint_message;

        if (!this->crypto->verify(envelope) || !this->is_peer(envelope.sender()) || !checkpoint_message.ParseFromString(envelope.checkpoint_msg()))
        {
            LOG (error) << "Checkpoint validation failure - unable to verify envelope";
            continue;
        }
        checkpoint_hashes[checkpoint_t(checkpoint_message.sequence(), checkpoint_message.state_hash())].insert(envelope.sender());
    }

    //  filter checkpoint_hashes to only those pairs (checkpoint, senders) such that |senders| >= 2f+1
    std::map<bzn::checkpoint_t , std::set<bzn::uuid_t>> retval;
    std::copy_if(checkpoint_hashes.begin(), checkpoint_hashes.end(), std::inserter(retval, retval.end())
        , [&](const auto& h) {return h.second.size() >= this->honest_member_size(this->peers_beacon->current()->size());});

    return retval;
}

bool
pbft::is_valid_prepared_proof(const prepared_proof& proof, uint64_t valid_checkpoint_sequence) const
{
    const bzn_envelope& pre_prepare_envelope{proof.pre_prepare()};

    if (!this->is_peer(pre_prepare_envelope.sender()) || !this->crypto->verify(pre_prepare_envelope))
    {
        LOG(error) << "is_valid_prepared_proof - a pre prepare message has a bad envelope, or the sender is not in the peers list";
        LOG(error) << "Sender: " << pre_prepare_envelope.sender() << " is " << (this->is_peer(pre_prepare_envelope.sender()) ? "" : "not ") << "a peer";
        return false;
    }

    pbft_msg preprepare_message;
    if (!preprepare_message.ParseFromString(pre_prepare_envelope.pbft()) || (preprepare_message.sequence() <= valid_checkpoint_sequence)
        || preprepare_message.type() != PBFT_MSG_PREPREPARE)
    {
        LOG(error) << "is_valid_prepared_proof - a pre prepare message has an invalid sequence number, or is malformed";
        return false;
    }

    std::set<uuid_t> senders;
    for (int j{0}; j < proof.prepare_size(); ++j)
    {
        bzn_envelope prepare_envelope{proof.prepare(j)};
        if (!this->is_peer(prepare_envelope.sender()) || !this->crypto->verify(prepare_envelope))
        {
            LOG(error) << "is_valid_prepared_proof - a prepare message has a bad envelope, "
                          "the sender may not be in the peer list, or the envelope failed cryptographic verification";
            LOG(error) << "Sender: " << pre_prepare_envelope.sender() << " is " << (this->is_peer(pre_prepare_envelope.sender()) ? "" : "not ") << "a peer";
            return false;
        }

        // does the sequence number, view number and hash match those of the pre prepare
        pbft_msg prepare_msg;
        if (!prepare_msg.ParseFromString(prepare_envelope.pbft()) || prepare_msg.type() != PBFT_MSG_PREPARE)
        {
            LOG(error) << "is_valid_prepared_proof - a prepare message is invalid";
            return false;
        }

        if (preprepare_message.sequence() != prepare_msg.sequence() || preprepare_message.view() != prepare_msg.view()
            || preprepare_message.request_hash() != prepare_msg.request_hash())
        {
            LOG(error) << "is_valid_prepared_proof - a pre prepare message has mismatched sequence, view or request hash";
            return false;
        }

        senders.insert(prepare_envelope.sender());
    }

    if (senders.size() < 2 * this->max_faulty_nodes() + 1)
    {
        LOG(error) << "is_valid_prepared_proof - not enough prepares in a prepared_proof";
        return false;
    }

    return true;
}

bool
pbft::is_valid_viewchange_message(const pbft_msg& viewchange_message, const bzn_envelope& original_msg) const
{
    if (!this->is_peer(original_msg.sender()))// TODO: Rich - If redundant, keep this check, remove the other
    {
        LOG(debug) << "is_valid_viewchange_message - message not from a peer";
        return false;
    }

    if (!(viewchange_message.view() > this->get_view()))
    {
        LOG(error) << "is_valid_viewchange_message - new view " << viewchange_message.view() << " is not greater than current view " << this->get_view();
        return false;
    }

    auto valid_checkpoint_hashes{this->validate_and_extract_checkpoint_hashes(viewchange_message)};
    if (valid_checkpoint_hashes.empty() && viewchange_message.sequence() != 0)
    {
        LOG(error) << "is_valid_viewchange_message - the checkpoint is invalid";
        return false;
    }

    // If we do not have a valid checkpoint hash, then we must set the valid_checkpoint_sequence value to 0.
    uint64_t valid_checkpoint_sequence = (valid_checkpoint_hashes.empty() ? 0 : valid_checkpoint_hashes.begin()->first.first);

    // Snuck into KEP-902: Isabel notes that viewchange messages should not have sequences, and this will be  refactored
    // out in the near future, but since we are using it we must make the comparison to the expected
    // valid_checkpoint_sequence
    if (viewchange_message.sequence() != valid_checkpoint_sequence)
    {
        LOG(error) << "is_valid_viewchange_message - the viewchange sequence: " << viewchange_message.sequence()
            << ", is different from the sequence that is expected: " <<  valid_checkpoint_sequence;
        return false;
    }

    // all the the prepared proofs are valid  (contains a pre prepare and 2f+1 mathcin prepares)
    for (int i{0}; i < viewchange_message.prepared_proofs_size(); ++i)
    {
        if (!this->is_valid_prepared_proof(viewchange_message.prepared_proofs(i), valid_checkpoint_sequence)){
            LOG(error) << "is_valid_viewchange_message - prepared proof is invalid";
            return false;
        }
    }
    return true;
}

bool
pbft::get_sequences_and_request_hashes_from_proofs(
        const pbft_msg& viewchange_msg
        , std::set<std::pair<uint64_t, std::string>>& sequence_request_pairs) const
{
    for (int j{0}; j < viewchange_msg.prepared_proofs_size(); ++j)
    {
        // --- add the sequence/hash to a set
        auto const& prepared_proof = viewchange_msg.prepared_proofs(j);

        pbft_msg msg;
        if (const bzn_envelope& envelope{prepared_proof.pre_prepare()}; !this->is_peer(envelope.sender()) || !msg.ParseFromString(envelope.pbft()))
        {
            return false;
        }
        sequence_request_pairs.insert(std::make_pair(msg.sequence(), msg.request_hash()));
    }
    return true;
}

bool
pbft::is_valid_newview_message(const pbft_msg& theirs, const bzn_envelope& original_theirs) const
{
    // - does it contain 2f+1 viewchange messages
    if (static_cast<size_t>(theirs.viewchange_messages_size()) < 2 * this->max_faulty_nodes() + 1)
    {
        return false;
    }

    std::set<std::string> viewchange_senders;
    pbft_msg viewchange_msg;
    for (int i{0};i < theirs.viewchange_messages_size();++i)
    {
        const bzn_envelope& original_msg{theirs.viewchange_messages(i)};
        // - are each of those viewchange messages valid?
        if (!viewchange_msg.ParseFromString(original_msg.pbft()) || viewchange_msg.type() != PBFT_MSG_VIEWCHANGE
            || !this->is_valid_viewchange_message(viewchange_msg, original_msg))
        {
            LOG(error) << "is_valid_newview_message - new view message contains invalid viewchange message";
            return false;
        }

        if (viewchange_msg.view() != theirs.view())
        {
            LOG(error) << "is_valid_newview_message - a view change message has a different view than the new view message";
            return false;
        }
        viewchange_senders.insert(original_msg.sender());
    }

    if (viewchange_senders.size() != size_t(theirs.viewchange_messages_size()))
    {
        LOG (error) << "is_valid_newview_message - unexpected viewchange message count";
        return false;
    }

    std::map<uuid_t,bzn_envelope> viewchange_envelopes_from_senders;

    for (int i{0};i < theirs.viewchange_messages_size();++i)
    {
        auto const& viewchange_env = theirs.viewchange_messages(i);
        viewchange_envelopes_from_senders[viewchange_env.sender()] = viewchange_env;
    }

    pbft_msg ours = this->build_newview(theirs.view(), viewchange_envelopes_from_senders, false);
    if (ours.pre_prepare_messages_size() != theirs.pre_prepare_messages_size())
    {
        LOG(error) << "is_valid_newview_message - expected " << ours.pre_prepare_messages_size()
            << " preprepares in new view, found " << theirs.pre_prepare_messages_size();
        return false;
    }

    for (int i{0};i < theirs.pre_prepare_messages_size();++i)
    {
        if (!this->crypto->verify(theirs.pre_prepare_messages(i)))
        {
            LOG(error) <<  "is_valid_newview_message - unable to verify thier pre prepare message";
            return false;
        }

        if (theirs.pre_prepare_messages(i).sender() != original_theirs.sender())
        {
            LOG(error) << "is_valid_newview_message - pre prepare messaged does not come from the correct sender";
            return false;
        }

        pbft_msg ours_pbft;
        pbft_msg theirs_pbft;

        ours_pbft.ParseFromString(ours.pre_prepare_messages(i).pbft());
        theirs_pbft.ParseFromString(theirs.pre_prepare_messages(i).pbft());

        if (ours_pbft.type() != theirs_pbft.type())
        {
            LOG(error) << "Invalid new view message: (ours_pbft.type() != theirs_pbft.type()): ours:[" <<  ours_pbft.type() << "] theirs:[" << theirs_pbft.type() << "]";
            return false;
        }
        else if (theirs_pbft.type() != PBFT_MSG_PREPREPARE)
        {
            LOG(error) << "Invalid new view message: (theirs_pbft.type() != PBFT_MSG_PREPREPARE): [" << theirs_pbft.type() << "]";
            return false;
        }
        else if (ours_pbft.view() != theirs_pbft.view())
        {
            LOG(error) << "Invalid new view message: (ours_pbft.view() != theirs_pbft.view()):  ours:[" << ours_pbft.view() << "] theirs:["  << theirs_pbft.view() << "]";
            return false;
        }
        else if (ours_pbft.sequence() != theirs_pbft.sequence())
        {
            LOG(error) << "Invalid new view message: ours_pbft.sequence() != theirs_pbft.sequence(): ours sequence:[" << ours_pbft.sequence() << "]  theirs:[" << theirs_pbft.sequence() << "]" ;
            return false;
        }
        else if (ours_pbft.request_hash() != theirs_pbft.request_hash())
        {
            LOG(error) << "Invalid new view message: ours_pbft.request_hash() != theirs_pbft.request_hash(): ours: [" << ours_pbft.request_hash() << "] theirs:[" << theirs_pbft.request_hash() << "]";
            return false;
        }
    }
    return true;
}

void
pbft::fill_in_missing_pre_prepares(uint64_t max_checkpoint_sequence, uint64_t new_view, std::map<uint64_t, bzn_envelope>& pre_prepares) const
{
    uint64_t last_sequence_number{0};
    for(const auto& pre_prepare : pre_prepares)
    {
        last_sequence_number = std::max(last_sequence_number, pre_prepare.first);
    }

    for (uint64_t i = max_checkpoint_sequence + 1; i <= last_sequence_number; ++i)
    {
        //  -- create a new preprepare for a no-op operation using this sequence number
        if (pre_prepares.find(i) == pre_prepares.end())
        {
            database_msg msg;
            msg.set_allocated_nullmsg(new database_nullmsg);

            auto request = new bzn_envelope;
            request->set_database_msg(msg.SerializeAsString());
            this->crypto->sign(*request);

            pbft_msg msg2;
            msg2.set_view(new_view);
            msg2.set_sequence(i);
            msg2.set_type(PBFT_MSG_PREPREPARE);
            msg2.set_request_hash(this->crypto->hash(*request));

            pre_prepares[i] = this->wrap_message(msg2);
            *(pre_prepares[i].add_piggybacked_requests()) = *request;
        }
    }
}

pbft_msg
pbft::make_newview(
        uint64_t new_view_index
        , const std::map<uuid_t,bzn_envelope>& viewchange_envelopes_from_senders
        , const std::map<uint64_t, bzn_envelope>& pre_prepare_messages
) const
{
    pbft_msg newview;
    newview.set_type(PBFT_MSG_NEWVIEW);
    newview.set_view(new_view_index);

    // V is the set of 2f+1 view change messages
    for (const auto &sender_viewchange_envelope: viewchange_envelopes_from_senders)
    {
        *(newview.add_viewchange_messages()) = sender_viewchange_envelope.second;
    }

    // O
    for (const auto& preprepare_message : pre_prepare_messages)
    {
        *(newview.add_pre_prepare_messages()) = preprepare_message.second;
    }
    return newview;
}

pbft_msg
pbft::build_newview(uint64_t new_view, const std::map<uuid_t,bzn_envelope>& viewchange_envelopes_from_senders
    , bool attach_reqs) const
{
    //  Computing O (set of new preprepares for new-view message)
    std::map<uint64_t, bzn_envelope> pre_prepares;

    uint64_t max_checkpoint_sequence{0};
    for (const auto& sender_viewchange_envelope : viewchange_envelopes_from_senders)
    {
        pbft_msg viewchange;
        viewchange.ParseFromString(sender_viewchange_envelope.second.pbft());
        max_checkpoint_sequence = std::max(max_checkpoint_sequence, viewchange.sequence());
    }

    //  - for each of the 2f+1 viewchange messages
    for (const auto& sender_viewchange_envelope : viewchange_envelopes_from_senders)
    {
        //  -- for each operation included in that viewchange message
        pbft_msg viewchange_message;
        viewchange_message.ParseFromString(sender_viewchange_envelope.second.pbft());

        for (int i{0}; i < viewchange_message.prepared_proofs_size(); ++i)
        {
            const auto& prepared_proof = viewchange_message.prepared_proofs(i);

            //  --- if we have not already created a new preprepare for an operation with that sequence number

            //  ---- Create a new preprepare for that operation using its original sequence number
            //       and request hash, but using the new view number
            pbft_msg pre_prepare;
            pre_prepare.ParseFromString(prepared_proof.pre_prepare().pbft());

            if (pre_prepares.count(pre_prepare.sequence())>0)
            {
                continue;
            }

            uint64_t old_view = pre_prepare.view();
            pre_prepare.set_view(new_view);

            if (pre_prepare.sequence() <= max_checkpoint_sequence)
            {
                continue;
            }

            auto env = this->wrap_message(pre_prepare);
            if (pre_prepare.request_type() == pbft_request_type::PBFT_REQUEST_PAYLOAD && attach_reqs)
            {
                auto op = this->operation_manager->find_or_construct(old_view, pre_prepare.sequence()
                    , pre_prepare.request_hash());
                if (op->has_request())
                {
                    *(env.add_piggybacked_requests()) = op->get_request();
                }
                else
                {
                    LOG(error) << "No request found for operation " << pre_prepare.sequence() << " in view "
                            << old_view;
                }
            }
            pre_prepares[pre_prepare.sequence()] = env;
        }
    }
    this->fill_in_missing_pre_prepares(max_checkpoint_sequence, new_view, pre_prepares);

    return this->make_newview(new_view, viewchange_envelopes_from_senders, pre_prepares);
}

void
pbft::save_checkpoint(const pbft_msg& msg)
{
    for (int i{0}; i < msg.checkpoint_messages_size(); ++i)
    {
        const bzn_envelope& original_checkpoint{msg.checkpoint_messages(i)};

        if (!this->crypto->verify(original_checkpoint))
        {
            LOG(error) << "ignoring invalid checkpoint message";
            continue;
        }

        this->checkpoint_manager->handle_checkpoint_message(original_checkpoint);
    }
}


std::map<bzn::hash_t, int>
pbft::map_request_to_hash(const bzn_envelope& env)
{
    std::map<bzn::hash_t, int> piggybacked_request_hashes;
    for (int i{0}; i < env.piggybacked_requests_size(); ++i)
    {
        const bzn_envelope request{env.piggybacked_requests(i)};
        const auto hash{this->crypto->hash(request)};
        piggybacked_request_hashes[hash] = i;
    }
    return piggybacked_request_hashes;
}


void
pbft::save_all_requests(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    const auto piggybacked_request_hashes{this->map_request_to_hash(original_msg)};

    // go through the prepared proofs
    for (int i{0}; i < msg.prepared_proofs_size(); ++i)
    {
        const auto &prepared_proof = msg.prepared_proofs(i);

        // look at the pre-prepare for each proof
        if (prepared_proof.has_pre_prepare())
        {
            const bzn_envelope &pre_prepare{prepared_proof.pre_prepare()};

            pbft_msg pp_msg;
            pp_msg.ParseFromString(pre_prepare.pbft());

            const hash_t& hash = pp_msg.request_hash();

            // find the piggybacked request in original_msg that corresponds to the hash
            const auto piggybacked_request_it{piggybacked_request_hashes.find(hash)};

            // if you find one, then save the request in the appropriate operation, because we used map::find, we know
            // that if we got a mapo::end then we didn't find a request
            if (piggybacked_request_it != piggybacked_request_hashes.end())
            {
                const bzn_envelope &request_env{original_msg.piggybacked_requests(piggybacked_request_it->second)};

                const uint64_t& pre_prep_view{pp_msg.view()};

                auto op = this->operation_manager->find_or_construct(
                        pre_prep_view, pp_msg.sequence(), pp_msg.request_hash());

                op->record_request(request_env);
            }
        }
    }
}


void
pbft::handle_viewchange(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    if (!this->is_valid_viewchange_message(msg, original_msg))
    {
        LOG(error) << "handle_viewchange - invalid viewchange message, ignoring";
        return;
    }

    save_all_requests(msg, original_msg);

    auto mutable_env{original_msg};
    mutable_env.clear_piggybacked_requests();

    this->valid_viewchange_messages_for_view[msg.view()][original_msg.sender()] =
        {this->storage, mutable_env, VALID_VIEWCHANGE_MESSAGES_FOR_VIEW_KEY, original_msg.sender(), msg.view()};

    this->save_checkpoint(msg);

    if (this->is_view_valid() &&
        this->valid_viewchange_messages_for_view[msg.view()].size() == this->max_faulty_nodes() + 1 &&
        msg.view() > this->last_view_sent.value())
    {
        this->last_view_sent = msg.view();
        this->initiate_viewchange();
    }

    const auto viewchange = std::find_if(this->valid_viewchange_messages_for_view.begin()
        , this->valid_viewchange_messages_for_view.end(), [&](const auto& p)
        {
            return ((this->get_primary(p.first).uuid == this->get_uuid()) &&
                (p.first > this->view.value()) &&
                (p.second.size() == 2 * this->max_faulty_nodes() + 1));
        });

    if (viewchange == this->valid_viewchange_messages_for_view.end())
    {
        return;
    }

    std::map<uuid_t, bzn_envelope> viewchange_envelopes_from_senders;
    for (const auto &sender_envelope : this->valid_viewchange_messages_for_view[msg.view()])
    {
        const auto &sender{sender_envelope.first};
        const auto &viewchange_envelope{sender_envelope.second.value()};
        viewchange_envelopes_from_senders[sender] = viewchange_envelope;
    }

    auto newview_msg = this->build_newview(viewchange->first, viewchange_envelopes_from_senders);
    LOG(debug) << "Sending NEWVIEW for view " << this->view.value() + 1;
    this->async_signed_broadcast(newview_msg);
}

void
pbft::handle_newview(const pbft_msg& msg, const bzn_envelope& original_msg)
{
    if (!this->is_valid_newview_message(msg, original_msg))
    {
        LOG (debug) << "handle_newview - ignoring invalid NEWVIEW message ";
        return;
    }

    pbft_msg viewchange;
    viewchange.ParseFromString(msg.viewchange_messages(0).pbft());

    // this is redundant (but harmless) unless it's a new node
    this->save_checkpoint(viewchange);

    // initially set next sequence from viewchange then update from preprepares later
    this->next_issued_sequence_number = viewchange.sequence() + 1;

    LOG(debug) << "handle_newview - received valid NEWVIEW message";

    this->view = msg.view();
    this->view_is_valid = true;

    // after moving to the new view processes the preprepares
    for (size_t i{0}; i < static_cast<size_t>(msg.pre_prepare_messages_size()); ++i)
    {
        const bzn_envelope original_msg2 = msg.pre_prepare_messages(i);
        pbft_msg msg2;
        if (msg2.ParseFromString(original_msg2.pbft()))
        {
            this->handle_preprepare(msg2, original_msg2);

            this->next_issued_sequence_number = msg2.sequence() + 1;
        }
    }

    // save the newview message for providing with state messages
    this->saved_newview = original_msg;
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
    std::string status_str;

    std::lock_guard<std::mutex> lock(this->pbft_lock);

    status_str += "my uuid: " + this->uuid + "\n";
    status_str += "primary: " + this->get_primary(this->get_view()).uuid + "\n";
    status_str += "view: " + std::to_string(this->get_view()) + "\n";
    status_str += "last exec: "  + std::to_string(this->last_executed_sequence_number) + "\n";
    status_str += "last local cp: "  + std::to_string(this->checkpoint_manager->get_latest_local_checkpoint().first) + "\n";
    status_str += "last stable cp: "  + std::to_string(this->checkpoint_manager->get_latest_stable_checkpoint().first) + "\n";
    if (this->is_primary())
    {
        status_str += "next seq: " + std::to_string(this->next_issued_sequence_number.value());
    }
    LOG(debug) << "status:\n" << status_str;

    status["outstanding_operations_count"] = uint64_t(this->operation_manager->held_operations_count());
    status["is_primary"] = this->is_primary();

    auto primary = this->get_primary();
    status["primary"]["host"] = primary.host;
    status["primary"]["host_port"] = primary.port;
    status["primary"]["name"] = primary.name;
    status["primary"]["uuid"] = primary.uuid;

    status["latest_stable_checkpoint"]["sequence_number"] = this->checkpoint_manager->get_latest_stable_checkpoint().first;
    status["latest_stable_checkpoint"]["hash"] = this->checkpoint_manager->get_latest_stable_checkpoint().second;
    status["latest_checkpoint"]["sequence_number"] = this->checkpoint_manager->get_latest_local_checkpoint().first;
    status["latest_checkpoint"]["hash"] = this->checkpoint_manager->get_latest_local_checkpoint().second;

    status["next_issued_sequence_number"] = this->next_issued_sequence_number.value();
    status["view"] = this->view.value();

    status["peer_index"] = bzn::json_message();
    for (const auto& p : *this->peers_beacon->current())
    {
        bzn::json_message peer;
        peer["host"] = p.host;
        peer["port"] = p.port;
        peer["name"] = p.name;
        peer["uuid"] = p.uuid;
        status["peer_index"].append(peer);
    }

    return status;
}

const peer_address_t&
pbft::get_peer_by_uuid(const std::string& uuid) const
{
    for (auto const& peer : *this->peers_beacon->current())
    {
        if (peer.uuid == uuid)
        {
            return peer;
        }
    }

    // something went wrong. this uuid should exist
    throw std::runtime_error("peer missing from peers list");
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

    this->recent_requests.erase(this->recent_requests.begin(),
        this->recent_requests.upper_bound(this->now() - MAX_REQUEST_AGE_MS));
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

std::shared_ptr<bzn_envelope>
pbft::make_viewchange(
        uint64_t new_view
        , uint64_t base_sequence_number
        , const std::unordered_map<bzn::uuid_t, std::string>& stable_checkpoint_proof
        , const std::map<uint64_t, std::shared_ptr<bzn::pbft_operation>>& prepared_operations)
{
    auto env = std::make_shared<bzn_envelope>();
    pbft_msg viewchange;

    viewchange.set_type(PBFT_MSG_VIEWCHANGE);
    viewchange.set_view(new_view);
    viewchange.set_sequence(base_sequence_number);  // base_sequence_number = n = sequence # of last valid checkpoint

    // C = a set of local 2*f + 1 valid checkpoint messages
    for (const auto& msg : stable_checkpoint_proof)
    {
        bzn_envelope envelope;
        envelope.ParseFromString(msg.second);
        *(viewchange.add_checkpoint_messages()) = envelope;
    }

    // P = a set (of client requests) containing a set P_m  for each request m that prepared at i with a sequence # higher than n
    // P_m = the pre prepare and the 2 f + 1 prepares
    //            get the set of operations, from each operation get the messages..
    for (const auto& operation : prepared_operations)
    {
        prepared_proof* prepared_proof = viewchange.add_prepared_proofs();
        auto pre_prepare = operation.second->get_preprepare();
        if (!operation.second->has_config_request())
        {
            auto req = operation.second->get_request();
            *(env->add_piggybacked_requests()) = req;
        }

        prepared_proof->set_allocated_pre_prepare(new bzn_envelope(pre_prepare));
        for (const auto& sender_envelope : operation.second->get_prepares())
        {
            *(prepared_proof->add_prepare()) = sender_envelope.second;
        }
    }

    env->set_pbft(viewchange.SerializeAsString());
    return env;
}

size_t
pbft::faulty_nodes_bound(size_t swarm_size)
{
    return (swarm_size - 1)/3;
}

size_t
pbft::honest_member_size(size_t swarm_size)
{
    return pbft::faulty_nodes_bound(swarm_size) + 1;
}

size_t
pbft::honest_majority_size(size_t swarm_size)
{
    return pbft::faulty_nodes_bound(swarm_size) * 2 + 1;
}

uint32_t
pbft::generate_random_number(uint32_t min, uint32_t max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(min, max);
    return dist(gen);
}

void pbft::add_session_to_sessions_waiting(const std::string& msg_hash, std::shared_ptr<bzn::session_base> session)
{
    if (session)
    {
        LOG(debug) << "Holding session for request " << bzn::bytes_to_debug_string(msg_hash);
        this->sessions_waiting_on_forwarded_requests[msg_hash] = session;
        session->add_shutdown_handler([msg_hash, this, session]()
        {
            std::lock_guard<std::mutex> lock(this->pbft_lock);
            auto it = this->sessions_waiting_on_forwarded_requests.find(msg_hash);
            if (it != this->sessions_waiting_on_forwarded_requests.end()
                && (it->second->get_session_id() == session->get_session_id()))
            {
                this->sessions_waiting_on_forwarded_requests.erase(it);
            }
        });
    }
}

void
pbft::initialize_persistent_state()
{
    persistent<operation_key_t>::init_kv_container<log_key_t>(this->storage, ACCEPTED_PREPREPARES_KEY,
        this->accepted_preprepares);
    persistent<bzn_envelope>::init_kv_container2<uuid_t, uint64_t>(this->storage,
        VALID_VIEWCHANGE_MESSAGES_FOR_VIEW_KEY, this->valid_viewchange_messages_for_view);
}

checkpoint_t
pbft::latest_stable_checkpoint() const
{
    return this->checkpoint_manager->get_latest_stable_checkpoint();
}

std::shared_ptr<bzn::peers_beacon_base>
pbft::peers() const
{
    return this->peers_beacon;
}
