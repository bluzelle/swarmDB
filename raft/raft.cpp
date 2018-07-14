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

#include <raft/raft.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <string>
#include <random>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <proto/bluzelle.pb.h>

namespace
{
    const std::string NO_PEERS_ERRORS_MGS{"No peers given!"};
    const std::string MSG_ERROR_INVALID_LOG_ENTRY_FILE{"Invalid log entry file. Please Delete .state folder."};
    const std::string MSG_ERROR_EMPTY_LOG_ENTRY_FILE{"Empty log entry file. Please delete .state folder."};
    const std::string MSG_ERROR_INVALID_STATE_FILE{"Invalid state file. Please delete the .state folder."};
    const std::string MSG_NO_PEERS_IN_LOG{"Unable to find peers in log entries."};

    const std::chrono::milliseconds DEFAULT_HEARTBEAT_TIMER_LEN{std::chrono::milliseconds(250)};
    const std::chrono::milliseconds  DEFAULT_ELECTION_TIMER_LEN{std::chrono::milliseconds(1250)};

    const std::string RAFT_TIMEOUT_SCALE = "RAFT_TIMEOUT_SCALE";
}


using namespace bzn;


raft::raft(
        std::shared_ptr<bzn::asio::io_context_base> io_context
        , std::shared_ptr<bzn::node_base> node
        , const bzn::peers_list_t& peers
        , bzn::uuid_t uuid
        , const std::string state_dir
        , size_t maximum_raft_storage)
    : timer(io_context->make_unique_steady_timer())
    , uuid(std::move(uuid))
    , node(std::move(node))
    , state_dir(std::move(state_dir))
{
    // we must have a list of peers!
    if (peers.empty())
    {
        throw std::runtime_error(NO_PEERS_ERRORS_MGS);
    }
    this->setup_peer_tracking(peers);
    this->get_raft_timeout_scale();

    const std::string log_path(state_dir + "/" + this->uuid + ".dat");
    this->state_files_exist() ? this->load_state()
                               : this->create_state_files(log_path, peers);

    this->raft_log = std::make_shared<bzn::raft_log>(log_path, maximum_raft_storage);

    // KEP-112 Bail if the raft storage exceeds the max storage
    if(this->raft_log->maximum_storage_exceeded())
    {
        LOG(error) << MSG_ERROR_MAXIMUM_STORAGE_EXCEEDED;
        throw std::runtime_error(MSG_ERROR_MAXIMUM_STORAGE_EXCEEDED);
    }
}


void
raft::setup_peer_tracking(const bzn::peers_list_t& peers)
{
    for (const auto& peer : peers)
    {
        this->peer_match_index[peer.uuid] = 1;
    }
}


void
raft::get_raft_timeout_scale()
{
    // for testing we may want to slow things down...
    if (auto env = getenv(RAFT_TIMEOUT_SCALE.c_str()))
    {
        try
        {
            this->timeout_scale = std::max(1ul, std::stoul(env));
        }
        catch(std::exception& ex)
        {
            LOG(error) << "Invalid RAFT_TIMEOUT_SCALE value: " << env;
        }
    }

    LOG(debug) << "RAFT_TIMEOUT_SCALE: " << this->timeout_scale;
}


bzn::raft_state
raft::get_state()
{
    return this->current_state;
}


void
raft::start()
{
    std::call_once(this->start_once,
        [this]()
        {
            this->start_election_timer();

            this->node->register_for_message(
                    "raft"
                    , std::bind(&raft::handle_ws_raft_messages, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
        });
}


void
raft::start_election_timer()
{
    this->timer->cancel();

    std::random_device rd;
    std::mt19937 gen(rd());

    // todo: testing range as big messages can cause election to occur...
    std::uniform_int_distribution<uint32_t> dist(DEFAULT_HEARTBEAT_TIMER_LEN.count() * 3 * this->timeout_scale, DEFAULT_ELECTION_TIMER_LEN.count() * this->timeout_scale);

    auto timeout = std::chrono::milliseconds(dist(gen));

    LOG(info) << "election timer will expire in: " << timeout.count() << "ms";

    this->timer->expires_from_now(timeout);

    this->timer->async_wait(std::bind(&raft::handle_election_timeout, shared_from_this(), std::placeholders::_1));
}


void
raft::start_heartbeat_timer()
{
    this->timer->cancel();

    LOG(debug) << "heartbeat timer will expire in: " << DEFAULT_HEARTBEAT_TIMER_LEN.count() * this->timeout_scale << "ms";

    this->timer->expires_from_now(DEFAULT_HEARTBEAT_TIMER_LEN * this->timeout_scale);

    this->timer->async_wait(std::bind(&raft::handle_heartbeat_timeout, shared_from_this(), std::placeholders::_1));
}


void
raft::handle_election_timeout(const boost::system::error_code& ec)
{
    if (ec)
    {
        LOG(info) << "election timer was canceled: " << ec.message();

        return;
    }

    // request votes from our peer list...
    this->request_vote_request();
}


void
raft::request_vote_request()
{
    std::lock_guard<std::mutex> lock(this->raft_lock);

    // update raft state...
    this->voted_for = this->uuid;
    this->yes_votes.clear();
    this->yes_votes.emplace(this->uuid);
    this->no_votes.clear();
    ++this->current_term;
    this->update_raft_state(this->current_term, bzn::raft_state::candidate);
    this->leader.clear();

    for (const auto& peer : this->get_all_peers())
    {
        // skip ourselves...
        if (peer.uuid == this->uuid)
        {
            continue;
        }

        try
        {
            // todo: use resolver on hostname...
            auto ep = boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string(peer.host), peer.port};

            this->node->send_message(ep, std::make_shared<bzn::message>(bzn::create_request_vote_request(this->uuid, this->current_term, this->raft_log->size(), this->last_log_term)));
        }
        catch(const std::exception& ex)
        {
            LOG(error) << "could not send RequestVote request to peer: " << peer.name << " [" << ex.what() << "]";
        }
    }

    // restart the election timer in case we don't complete it in time...
    this->start_election_timer();
}


void
raft::handle_request_vote_response(const bzn::message& msg, std::shared_ptr<bzn::session_base> /*session*/)
{
    LOG(debug) << '\n' << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

    // If I'm the leader and my term is less than vote request then I should step down and become a follower.
    if (this->current_state == bzn::raft_state::leader)
    {
        if (this->current_term < msg["data"]["term"].asUInt())
        {
            LOG(error) << "vote from: " << msg["data"]["from"].asString() << " in wrong term: "
                       << msg["data"]["term"].asUInt();

            // reset ourselves to follower...
            this->update_raft_state(msg["data"]["term"].asUInt(), bzn::raft_state::follower);

            this->start_election_timer();

            return;
        }
    }

    if(this->current_state != bzn::raft_state::candidate)
    {
        LOG(warning) << "No longer a candidate. Ignoring message from peer: " << msg["data"]["from"].asString();

        return;
    }

    // tally the votes...
    if (msg["data"]["granted"].asBool())
    {
        this->yes_votes.emplace(msg["data"]["from"].asString());
        if(this->is_majority(this->yes_votes))
        {
            this->update_raft_state(this->current_term, bzn::raft_state::leader);

            // clear any previous peer state...
            for (auto& entry : this->peer_match_index)
            {
                entry.second = 1;
            }

            this->request_append_entries();

            return;
        }
    }
    else
    {
        this->no_votes.emplace(msg["data"]["from"].asString());
        if(this->is_majority(this->no_votes))
        {
            this->update_raft_state(this->current_term, bzn::raft_state::follower);

            this->start_election_timer();

            return;
        }
    }
}


void
raft::handle_ws_request_vote(const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
{
    if (this->current_state == bzn::raft_state::leader || this->voted_for)
    {
        session->send_message(std::make_shared<bzn::message>(bzn::create_request_vote_response(this->uuid, this->current_term, false)), true);

        return;
    }

    // vote for this peer...
    this->voted_for = msg["data"]["from"].asString();

    bool vote = msg["data"]["lastLogIndex"].asUInt() >= this->raft_log->size();

    session->send_message(std::make_shared<bzn::message>(bzn::create_request_vote_response(this->uuid, this->current_term, vote)), true);
}


void
raft::handle_ws_append_entries(const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
{
    uint32_t term = msg["data"]["term"].asUInt();

    if ((this->current_state == bzn::raft_state::candidate || this->current_state == bzn::raft_state::leader) &&
        this->current_term >= term)
    {
        LOG(debug) << "received append entries -- aborting election.";

        this->update_raft_state(term, bzn::raft_state::follower);
        this->start_election_timer();
        session->close();
        return;
    }

    this->leader = msg["data"]["from"].asString();

    bool success = false;
    uint32_t leader_prev_term  = msg["data"]["prevTerm"].asUInt();
    uint32_t leader_prev_index = msg["data"]["prevIndex"].asUInt();
    uint32_t entry_term = msg["data"]["entryTerm"].asUInt();

    uint32_t msg_index = leader_prev_index + 1;

    // Accept the message if its previous index and previous term are consistent with our log
    if (this->raft_log->entry_accepted(leader_prev_index, leader_prev_term))
    {
        success = true;

        // Now if the message actually has data, and we don't have that data, we can append it.
        // If it has data but our log is longer, raft guarentees that the data is the same.
        if (!msg["data"]["entries"].empty() )
        {
            LOG(debug) << "Follower inserting entry with message index:" << msg_index;
            this->raft_log->follower_insert_entry(msg_index, log_entry{raft::deduce_type_from_message(msg["data"]["entries"]), msg_index, entry_term, msg["data"]["entries"]});
        }
    }
    else
    {
        // We don't agree with or don't have the previous index the leader thinks we have, so saying no will
        // tell the leader to send older data
        if (leader_prev_index < this->raft_log->size())
        {
            LOG(debug) << "Rejecting AppendEntries because I do not have the previous index";
        }
        else
        {
            LOG(debug) << "Rejecting AppendEntries because I do not agree with the previous index";
        }
        success = false;
    }

    size_t match_index = std::min(this->raft_log->size(), (size_t) msg_index+1);

    auto resp_msg = std::make_shared<bzn::message>(bzn::create_append_entries_response(this->uuid, this->current_term, success, match_index));

    LOG(debug) << "Sending WS message:\n" << resp_msg->toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

    session->send_message(resp_msg, true);

    // update commit index...
    if (success)
    {
        if (this->commit_index < msg["data"]["commitIndex"].asUInt())
        {
            for(size_t i = this->commit_index; i < std::min(this->raft_log->size(), size_t(msg["data"]["commitIndex"].asUInt())); ++i)
            {
                this->perform_commit(commit_index, this->raft_log->entry_at(i));
            }
        }
    }

    // update leader's peer index
    this->peer_match_index[this->leader] = msg["data"]["commitIndex"].asUInt();

    this->start_election_timer();
}


bzn::message
raft::create_joint_quorum_by_adding_peer(const bzn::message& last_quorum_message, const bzn::message& new_peer)
{
    bzn::message joint_quorum;
    bzn::message peers;
    peers["old"] = peers["new"] = last_quorum_message["msg"]["peers"];
    peers["new"].append(new_peer);
    joint_quorum["msg"]["peers"] = peers;
    return joint_quorum;
}


bzn::message
raft::create_single_quorum_from_joint_quorum(const bzn::message& joint_quorum)
{
    bzn::message single_quorum;
    single_quorum["msg"]["peers"] = joint_quorum["msg"]["peers"]["new"];
    return single_quorum;
}


bzn::message
raft::create_joint_quorum_by_removing_peer(const bzn::message& last_quorum_message, const bzn::uuid_t& peer_uuid)
{
    bzn::message joint_quorum;
    const auto& peers = last_quorum_message["msg"]["peers"];
    joint_quorum["msg"]["peers"]["old"] = peers;

    std::for_each(peers.begin(), peers.end(),
                  [&](const auto& p)
                  {
                      if(p["uuid"]!=peer_uuid)
                      {
                          joint_quorum["msg"]["peers"]["new"].append(p);
                      }
                  });
    return joint_quorum;
}


void
raft::handle_add_peer(std::shared_ptr<bzn::session_base> session, const bzn::message &peer)
{
    if (this->get_state() != bzn::raft_state::leader)
    {
        bzn::message response;
        response["error"] = ERROR_ADD_PEER_MUST_BE_SENT_TO_LEADER;
        session->send_message(std::make_shared<bzn::message>(response), true);
        return;
    }

    bzn::log_entry last_quorum_entry = this->raft_log->last_quorum_entry();
    if (last_quorum_entry.entry_type == bzn::log_entry_type::joint_quorum)
    {
        bzn::message response;
        response["error"] = MSG_ERROR_CURRENT_QUORUM_IS_JOINT;
        session->send_message(std::make_shared<bzn::message>(response), true);
        return;
    }

    const auto &same_peer = std::find_if(last_quorum_entry.msg["msg"]["peers"].begin(),
                                         last_quorum_entry.msg["msg"]["peers"].end(),
                                         [&](const auto &p)
                                         {
                                             return p["uuid"].asString() == peer["uuid"].asString();
                                         });
    if (same_peer != last_quorum_entry.msg["msg"]["peers"].end())
    {
        bzn::message response;
        response["error"] = ERROR_PEER_ALREADY_EXISTS;
        session->send_message(std::make_shared<bzn::message>(response), true);
        return;
    }

    if(!peer.isMember("uuid") || peer["uuid"].asString().empty())
    {
        bzn::message response;
        response["error"] = ERROR_INVALID_UUID;
        session->send_message(std::make_shared<bzn::message>(response), true);
        return;
    }

    this->peer_match_index[peer["uuid"].asString()] = 1;
    this->append_log_unsafe(this->create_joint_quorum_by_adding_peer(last_quorum_entry.msg, peer),
                            bzn::log_entry_type::joint_quorum);
    return;
}


void
raft::handle_remove_peer(std::shared_ptr<bzn::session_base> session, const std::string& uuid)
{
    if (this->get_state() != bzn::raft_state::leader)
    {
        bzn::message response;
        response["error"] = ERROR_REMOVE_PEER_MUST_BE_SENT_TO_LEADER;
        session->send_message(std::make_shared<bzn::message>(response), true);
        return;
    }

    bzn::log_entry last_quorum_entry = this->raft_log->last_quorum_entry();
    if (last_quorum_entry.entry_type == bzn::log_entry_type::joint_quorum)
    {
        bzn::message response;
        response["error"] = MSG_ERROR_CURRENT_QUORUM_IS_JOINT;
        session->send_message(std::make_shared<bzn::message>(response), true);
        return;
    }

    const auto &peers = last_quorum_entry.msg["msg"]["peers"];
    const auto &same_peer = std::find_if(peers.begin(), peers.end(),
            [&](const auto &peer)
            {
                return peer["uuid"].asString() == uuid;
            });
    if (same_peer == peers.end())
    {
        bzn::message response;
        response["error"] = ERROR_PEER_NOT_FOUND;
        session->send_message(std::make_shared<bzn::message>(response), true);
        return;
    }

    this->append_log_unsafe(this->create_joint_quorum_by_removing_peer(last_quorum_entry.msg, uuid),
                            bzn::log_entry_type::joint_quorum);
    return;
}


void
raft::handle_ws_raft_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
{
    std::lock_guard<std::mutex> lock(this->raft_lock);
    LOG(debug) << "Received WS message:\n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

    // TODO: refactor add/remove peers to move the functionality into handlers
    if(msg["cmd"].asString()=="add_peer")
    {
        this->handle_add_peer(session, msg["data"]["peer"]);
        return;
    }
    else if(msg["cmd"].asString() == "remove_peer" )
    {
        if (!msg.isMember("data") || !msg["data"].isMember("uuid") || msg["data"]["uuid"].asString().empty())
        {
            bzn::message response;
            response["error"] = ERROR_INVALID_UUID;
            session->send_message(std::make_shared<bzn::message>(response), true);
            return;
        }

        this->handle_remove_peer(session, msg["data"]["uuid"].asString());
        return;
    }

    // check that the message is from a node in the most recent quorum
    if(!in_quorum(msg["data"]["from"].asString()))
    {
        return;
    }

    uint32_t term = msg["data"]["term"].asUInt();

    if (this->current_term == term)
    {
        // bleh!
        if (msg["cmd"].asString() == "RequestVote")
        {
            this->handle_ws_request_vote(msg, session);
        }
        else if (msg["cmd"].asString() == "AppendEntries")
        {
            this->handle_ws_append_entries(msg, session);
        }
        else if (msg["cmd"].asString() == "AppendEntriesReply")
        {
            this->handle_request_append_entries_response(msg, session);
            session->close();
        }
        else if (msg["cmd"].asString() == "ResponseVote")
        {
            this->handle_request_vote_response(msg, session);
            session->close();
        }

        return;
    }
    else
    {
        // todo: We are the leader and we need to step down when term is out of sync?
        if (this->current_term < term)
        {
            this->current_term = msg["data"]["term"].asUInt();

            if (msg["cmd"].asString() == "RequestVote")
            {
                this->voted_for = msg["data"]["uuid"].asString();

                session->send_message(std::make_shared<bzn::message>(bzn::create_request_vote_response(this->uuid, this->current_term, true)), true);

                return;
            }

            if (msg["cmd"].asString() == "AppendEntries")
            {
                // TODO: We should either process this message properly, or just drop it after updating term
                this->leader = msg["data"]["from"].asString();

                auto resp_msg = std::make_shared<bzn::message>(bzn::create_append_entries_response(this->uuid, this->current_term, false, this->raft_log->size()));

                LOG(debug) << "Sending WS message:\n" << resp_msg->toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

                session->send_message(resp_msg, true);
            }

            LOG(info) << "current term out of sync: " << this->current_term;

            this->update_raft_state(this->current_term, bzn::raft_state::follower);
#ifndef __APPLE__
            this->voted_for.reset();
#else
            this->voted_for = std::experimental::optional<bzn::uuid_t>();
#endif
            this->start_election_timer();
            return;
        }

        // todo: drop back to follower and restart the election?
        if (this->current_term > term)
        {
            LOG(error) << "request had term out of sync: " << this->current_term << " > " << term;

            this->update_raft_state(msg["data"]["term"].asUInt(), bzn::raft_state::follower);
#ifndef __APPLE__
            this->voted_for.reset();
#else
            this->voted_for = std::experimental::optional<bzn::uuid_t>();
#endif
            this->start_election_timer();
            return;
        }
    }

    LOG(error) << "unhandled raft msg: " << msg["cmd"];
}


void
raft::handle_heartbeat_timeout(const boost::system::error_code& ec)
{
    if (ec)
    {
        LOG(debug) << "heartbeat timer was canceled: " << ec.message();

        return;
    }

    // request votes from our peer list...
    std::lock_guard<std::mutex> lock(this->raft_lock);

    this->request_append_entries();
    this->notify_leader_status();
}

void
raft::notify_leader_status()
{
    if(!this->enable_audit)
    {
        return;
    }

    audit_message msg;
    msg.mutable_leader_status()->set_term(this->current_term);
    msg.mutable_leader_status()->set_leader(this->uuid);
    msg.mutable_leader_status()->set_current_log_index(this->raft_log->size());
    msg.mutable_leader_status()->set_current_commit_index(this->commit_index);

    auto json_ptr = std::make_shared<bzn::message>();
    (*json_ptr)["bzn-api"] = "audit";
    (*json_ptr)["audit-data"] = boost::beast::detail::base64_encode(msg.SerializeAsString());

    for (const auto& peer : this->get_all_peers())
    {
        auto ep = boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string(peer.host), peer.port};
        this->node->send_message(ep, json_ptr);
    }
}

void
raft::notify_commit(size_t log_index, const std::string& operation)
{
    if(!this->enable_audit)
    {
        return;
    }

    audit_message msg;
    msg.mutable_commit()->set_log_index(log_index);
    msg.mutable_commit()->set_operation(operation);

    auto json_ptr = std::make_shared<bzn::message>();
    (*json_ptr)["bzn-api"] = "audit";
    (*json_ptr)["audit-data"] = boost::beast::detail::base64_encode(msg.SerializeAsString());


    for (const auto& peer : this->get_all_peers())
    {
        auto ep = boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string(peer.host), peer.port};
        this->node->send_message(ep, json_ptr);
    }
}

void
raft::request_append_entries()
{
    for (const auto& peer : this->get_all_peers())
    {
        // skip ourselves...
        if (peer.uuid == this->uuid)
        {
            continue;
        }

        try
        {
            bzn::message msg;
            uint32_t prev_index{};
            uint32_t prev_term{};
            uint32_t entry_term{};

            if (this->peer_match_index[peer.uuid] < this->raft_log->size())
            {
                auto index = this->peer_match_index[peer.uuid];
                prev_index = index-1;
                prev_term = this->raft_log->entry_at(prev_index).term;

                msg = this->raft_log->entry_at(index).msg;
                entry_term = this->raft_log->entry_at(index).term;
            }
            else
            {
                prev_index = this->raft_log->size()-1;
                prev_term = this->raft_log->entry_at(prev_index).term;
            }

            // todo: use resolver on hostname...
            auto ep = boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string(peer.host), peer.port};

            auto req = std::make_shared<bzn::message>(bzn::create_append_entries_request(this->uuid, this->current_term, this->commit_index, prev_index, prev_term, entry_term, msg));

            LOG(debug) << "Sending request:\n" << req->toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

            this->node->send_message(ep, req);
        }
        catch(const std::exception& ex)
        {
            LOG(error) << "could not send AppendEntries request to peer: " << peer.name << " [" << ex.what() << "]";
        }
    }

    // restart the heartbeat timer...
    this->start_heartbeat_timer();
}


void
raft::handle_request_append_entries_response(const bzn::message& msg, std::shared_ptr<bzn::session_base> /*session*/)
{
    if (this->current_state != bzn::raft_state::leader)
    {
        LOG(warning) << "No longer the leader. Ignoring message from peer: " << msg["data"]["from"].asString();
        return;
    }

    // check match index for bad peers...
    if (msg["data"]["matchIndex"].asUInt() > this->raft_log->size() ||
        msg["data"]["term"].asUInt() != this->current_term)
    {
        LOG(error) << "received bad match index or term: \n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";
        return;
    }

    this->peer_match_index[msg["data"]["from"].asString()] = msg["data"]["matchIndex"].asUInt();

    if (!msg["data"]["success"].asBool())
    {
        LOG(debug) << "append entry failed for peer: " << msg["data"]["uuid"].asString();
        return;
    }

    while (this->commit_index < this->last_majority_replicated_log_index())
    {
        this->perform_commit(this->commit_index, this->raft_log->entry_at(this->commit_index));
    }
}


bool
raft::append_log_unsafe(const bzn::message& msg, const bzn::log_entry_type entry_type)
{
    if(this->raft_log->maximum_storage_exceeded())
    {
        LOG(error) << MSG_ERROR_MAXIMUM_STORAGE_EXCEEDED;
        std::raise(SIGINT);
    }

    if (this->current_state != bzn::raft_state::leader)
    {
        LOG(warning) << "not the leader, can't append log_entries!";
        return false;
    }

    LOG(debug) << "Appending " << log_entry_type_to_string(entry_type) << " to my log: " << msg.toStyledString();

    this->raft_log->leader_append_entry(log_entry{entry_type, uint32_t(this->raft_log->size()), this->current_term, msg});

    return true;
}


bool
raft::append_log(const bzn::message& msg, const bzn::log_entry_type entry_type)
{
    std::lock_guard<std::mutex> lock(this->raft_lock);
    return this->append_log_unsafe(msg, entry_type);
}


void
raft::register_commit_handler(bzn::raft_base::commit_handler handler)
{
    this->commit_handler = std::move(handler);
}


void
raft::update_raft_state(uint32_t term, bzn::raft_state state)
{
    this->current_state = state;
    this->current_term  = term;

    switch (this->current_state)
    {
        case bzn::raft_state::leader:
            LOG(info) << "RAFT State: Leader";
            this->leader = this->uuid;
            break;

        case bzn::raft_state::candidate:
            LOG(info) << "RAFT State: Candidate";
            break;

        case bzn::raft_state::follower:
            LOG(info) << "RAFT State: Follower";
            break;
    }
}


bzn::peer_address_t
raft::get_leader()
{
    std::lock_guard<std::mutex> lock(this->raft_lock);

    for(auto& peer : this->get_all_peers())
    {
        if (peer.uuid == this->leader)
        {
            return peer;
        }
    }

    return bzn::peer_address_t("",0,0,"","");
}


void
raft::initialize_storage_from_log(std::shared_ptr<bzn::storage_base> storage)
{
    for (const auto& log_entry : this->raft_log->get_log_entries())
    {
        if(log_entry.entry_type == bzn::log_entry_type::database)
        {
            const auto command = log_entry.msg["cmd"].asString();
            const auto db_uuid = log_entry.msg["db-uuid"].asString();
            const auto key = log_entry.msg["data"]["key"].asString();
            if (command == "create")
            {
                storage->create(db_uuid, key, log_entry.msg["data"]["value"].asString());
            }
            else if (command == "update")
            {
                storage->update(db_uuid, key, log_entry.msg["data"]["value"].asString());
            }
            else if (command == "delete")
            {
                storage->remove(db_uuid, key);
            }
        }
    }
}


std::string
raft::entries_log_path()
{
    // Refactored as the original version was resulting in double '/' occurring
    boost::filesystem::path out{this->state_dir};
    out.append(this->get_uuid()+ ".dat");
    return out.string();
}


std::string
raft::state_path()
{
    // Refactored as the original version was resulting in double '/' occurring
    boost::filesystem::path out{this->state_dir};
    out.append(this->get_uuid() + ".state");
    return out.string();
}


void
raft::save_state()
{
    boost::filesystem::path path{this->state_path()};

    if (!boost::filesystem::exists(path.parent_path()))
    {
        boost::filesystem::create_directories(path.parent_path());
    }

    std::ofstream os( path.string() ,std::ios::out | std::ios::binary);
    os << this->last_log_term << " " << this->commit_index << " " <<  this->current_term;
    os.close();
}


void
raft::load_state()
{
    this->last_log_term = std::numeric_limits<uint32_t>::max();
    this->commit_index = std::numeric_limits<uint32_t>::max();
    this->current_term = std::numeric_limits<uint32_t>::max();
    std::ifstream is(this->state_path(), std::ios::in | std::ios::binary);
    is >> this->last_log_term >> this->commit_index >> this->current_term;
    if (std::numeric_limits<uint32_t>::max()==this->last_log_term
        || std::numeric_limits<uint32_t>::max()==this->commit_index
        || std::numeric_limits<uint32_t>::max()==this->current_term)
    {
        throw std::runtime_error(MSG_ERROR_INVALID_STATE_FILE);
    }
    is.close();
}


void
raft::perform_commit(uint32_t& commit_index, const bzn::log_entry& log_entry)
{
    this->notify_commit(commit_index, log_entry.json_to_string(log_entry.msg));
    this->commit_handler(log_entry.msg);
    commit_index++;
    this->save_state();

    if(this->get_state() == bzn::raft_state::leader && log_entry.entry_type == bzn::log_entry_type::joint_quorum)
    {
        this->append_log_unsafe(
                this->create_single_quorum_from_joint_quorum(log_entry.msg),
                bzn::log_entry_type::single_quorum);
    }
}


uint32_t
raft::last_majority_replicated_log_index()
{
    uint32_t result = UINT32_MAX;

    for(const auto& uuids: this->get_active_quorum())
    {
        std::vector<size_t> match_indices;
        std::transform(uuids.begin(), uuids.end(),
                       std::back_inserter(match_indices),
                       [&](const auto& uuid) {
                           return this->peer_match_index[uuid];
                       });

        std::sort(match_indices.begin(), match_indices.end());
        uint32_t median = match_indices[uint32_t(std::ceil(match_indices.size()/2.0))];
        result = std::min(result, median);
    }

    return result;
}


std::list<std::set<bzn::uuid_t>>
raft::get_active_quorum()
{
    // TODO: cache this method when the active quorum is updated

    auto extract_uuid = [](const auto& node_json) -> bzn::uuid_t
        {
            return node_json["uuid"].asString();
        };

    bzn::log_entry log_entry = this->raft_log->last_quorum_entry();
    switch(log_entry.entry_type)
    {
        case bzn::log_entry_type::single_quorum:
            {
                const auto& peers = log_entry.msg["msg"]["peers"];
                std::set<bzn::uuid_t> result;
                std::transform(
                        peers.begin()
                        , peers.end()
                        , std::inserter(result, result.begin()),
                        extract_uuid);
                return std::list<std::set<bzn::uuid_t>>{result};
            }
            break;
        case bzn::log_entry_type::joint_quorum:
            {
                std::set<bzn::uuid_t> result_old, result_new;
                const auto& old_jpeers = log_entry.msg["msg"]["peers"]["old"];
                const auto& new_jpeers = log_entry.msg["msg"]["peers"]["new"];
                std::transform(old_jpeers.begin()
                        , old_jpeers.end()
                        , std::inserter(result_old, result_old.begin())
                        , extract_uuid);
            std::transform(new_jpeers.begin()
                    , new_jpeers.end()
                    , std::inserter(result_new, result_new.begin())
                    , extract_uuid);
            return std::list<std::set<bzn::uuid_t>>{result_old, result_new};
            }
            break;
        default:
            throw std::runtime_error("last_quorum gave something that's not a quorum");
    }
    return std::list<std::set<bzn::uuid_t>>();
}


bool
raft::is_majority(const std::set<bzn::uuid_t>& votes)
{
    for(const auto& s : this->get_active_quorum())
    {
        std::set<bzn::uuid_t> intersect;
        std::set_intersection(s.begin(), s.end(), votes.begin(), votes.end(),
                              std::inserter(intersect, intersect.begin()));

        if(intersect.size() * 2 <= s.size())
        {
            return false;
        }
    }

    return true;
}


bzn::peers_list_t
raft::get_all_peers()
{
    bzn::peers_list_t result;
    std::vector<std::reference_wrapper<bzn::message>> all_peers;
    bzn::log_entry log_entry = this->raft_log->last_quorum_entry();

    if(log_entry.entry_type == bzn::log_entry_type::single_quorum)
    {
        all_peers.emplace_back(log_entry.msg["msg"]["peers"]);
    }
    else
    {
        all_peers.emplace_back(log_entry.msg["msg"]["peers"]["old"]);
        all_peers.emplace_back(log_entry.msg["msg"]["peers"]["new"]);
    }

    for(auto jpeers : all_peers)
    {
        for(const bzn::message& p : jpeers.get())
        {
            result.emplace(p["host"].asString(),
                           uint16_t(p["port"].asUInt()),
                           uint16_t(p["http_port"].asUInt()),
                           p["name"].asString(),
                           p["uuid"].asString());
        }
    }
    return result;
}


bool
raft::in_quorum(const bzn::uuid_t& uuid)
{
    for(const auto q : this->get_active_quorum())
    {
        if (std::find(q.begin(), q.end(),uuid) != q.end())
        {
            return true;
        }
    }
    return false;
}


void
raft::create_state_files(const std::string& log_path, const bzn::peers_list_t& peers)
{
    const boost::filesystem::path path(log_path);
    if(!boost::filesystem::exists(path.parent_path()))
    {
        boost::filesystem::create_directories(path.parent_path());
    }

    bzn::message root;
    root["msg"] = bzn::message();
    for(const auto& p : peers)
    {
        bzn::message peer;
        peer["host"] = p.host;
        peer["port"] = p.port;
        peer["http_port"] = p.http_port;
        peer["name"] = p.name;
        peer["uuid"] = p.uuid;
        root["msg"]["peers"].append(peer);
    }

    const bzn::log_entry entry{bzn::log_entry_type::single_quorum, 0, 0, root};

    std::ofstream os( log_path ,std::ios::out | std::ios::binary);
    if(!os)
    {
        throw std::runtime_error(ERROR_UNABLE_TO_CREATE_LOG_FILE_FOR_WRITING + log_path);
    }
    os << entry;
    os.close();
    this->save_state();
}


void
raft::import_state_files()
{
    this->load_state();
    const auto& last_entry = this->raft_log->get_log_entries().back();
    if (last_entry.log_index!=this->raft_log->size()-1 || last_entry.term!=this->current_term)
    {
        throw std::runtime_error(MSG_ERROR_INVALID_LOG_ENTRY_FILE);
    }
}


bool
raft::state_files_exist()
{
    return boost::filesystem::exists(this->state_path())
           && boost::filesystem::exists(this->entries_log_path());
}


bzn::log_entry_type
raft::deduce_type_from_message(const bzn::message& message)
{
    if(message["msg"].isString())
    {
        return bzn::log_entry_type::database;
    }

    if(message["msg"]["peers"].isArray())
    {
        return bzn::log_entry_type::single_quorum;
    }

    if(message["msg"]["peers"].isMember("new"))
    {
        return bzn::log_entry_type::joint_quorum;
    }
    return bzn::log_entry_type::undefined;
}
