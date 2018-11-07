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
#include <proto/audit.pb.h>
#include <utils/crypto.hpp>
#include <utils/blacklist.hpp>

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

    std::mt19937 gen(std::time(0)); //Standard mersenne_twister_engine seeded with rd()

    // TODO: RHN - this should be templatized
    bzn::peers_list_t::const_iterator
    choose_any_one_of(const bzn::peers_list_t& all_peers)
    {
        if (all_peers.size()>0)
        {
            std::uniform_int_distribution<> dis(1, all_peers.size());
            auto it = all_peers.begin();
            std::advance(it, dis(gen) - 1);
            return it;
        }
        return all_peers.end();
    }
}


using namespace bzn;


raft::raft(
        std::shared_ptr<bzn::asio::io_context_base> io_context,
        std::shared_ptr<bzn::node_base> node,
        const bzn::peers_list_t& peers,
        bzn::uuid_t uuid,
        const std::string state_dir, size_t maximum_raft_storage,
        bool enable_peer_validation,
        const std::string& signed_key
        )
        :timer(io_context->make_unique_steady_timer())
        ,uuid(std::move(uuid))
        ,node(std::move(node))
        ,state_dir(std::move(state_dir))
        ,enable_peer_validation(enable_peer_validation)
        ,signed_key(signed_key)
{
    // we must have a list of peers!
    if (peers.empty())
    {
        throw std::runtime_error(NO_PEERS_ERRORS_MGS);
    }
    this->setup_peer_tracking(peers);
    this->get_raft_timeout_scale();

    if (!boost::filesystem::exists(this->entries_log_path()))
    {
        this->create_dat_file(this->entries_log_path(), peers);
    }

    this->raft_log = std::make_shared<bzn::raft_log>(this->entries_log_path(), maximum_raft_storage);

    this->shutdown_on_exceeded_max_storage(true);
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

    LOG(debug) << "election timer will expire in: " << timeout.count() << "ms";

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
    // A reason that we may be asking for an election is that we are not in the
    // swarm so no one is sending us messages, if we haven't already, do a
    // quick check.
    if(!this->in_a_swarm)
    {
        this->auto_add_peer_if_required();
    }

    if (ec)
    {
        LOG(debug) << "election timer was canceled: " << ec.message();

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

            this->node->send_message(ep, std::make_shared<bzn::json_message>(bzn::create_request_vote_request(this->uuid, this->current_term, this->raft_log->size(), this->last_log_term)));
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
raft::handle_request_vote_response(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> /*session*/)
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

    if (this->current_state != bzn::raft_state::candidate)
    {
        LOG(warning) << "No longer a candidate. Ignoring message from peer: " << msg["data"]["from"].asString();

        return;
    }

    // tally the votes...
    if (msg["data"]["granted"].asBool())
    {
        this->yes_votes.emplace(msg["data"]["from"].asString());
        if (this->is_majority(this->yes_votes))
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
        if (this->is_majority(this->no_votes))
        {
            this->update_raft_state(this->current_term, bzn::raft_state::follower);

            this->start_election_timer();

            return;
        }
    }
}


void
raft::handle_ws_request_vote(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session)
{
    if (this->current_state == bzn::raft_state::leader || this->voted_for)
    {
        session->send_message(std::make_shared<bzn::json_message>(bzn::create_request_vote_response(this->uuid, this->current_term, false)), true);

        return;
    }

    // vote for this peer...
    this->voted_for = msg["data"]["from"].asString();

    bool vote = msg["data"]["lastLogIndex"].asUInt() >= this->raft_log->size();

    session->send_message(std::make_shared<bzn::json_message>(bzn::create_request_vote_response(this->uuid, this->current_term, vote)), true);
}


void
raft::handle_ws_append_entries(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session)
{
    uint32_t term = msg["data"]["term"].asUInt();

    // We've received an append entries from another node, we are in
    // a swarm.
    if(!this->in_a_swarm && (msg["data"]["from"].asString() != this->get_uuid()))
    {
        LOG(debug) << "RAFT - just received an append entries - auto add peer is unecessary";
        this->in_a_swarm = true;
    }

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
        if (leader_prev_index >= this->raft_log->size())
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

    auto resp_msg = std::make_shared<bzn::json_message>(bzn::create_append_entries_response(this->uuid, this->current_term, success, match_index));

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


bzn::json_message
raft::create_joint_quorum_by_adding_peer(const bzn::json_message& last_quorum_message, const bzn::json_message& new_peer)
{
    bzn::json_message joint_quorum;
    bzn::json_message peers;
    peers["old"] = peers["new"] = last_quorum_message["msg"]["peers"];
    peers["new"].append(new_peer);
    joint_quorum["msg"]["peers"] = peers;
    return joint_quorum;
}


bzn::json_message
raft::create_single_quorum_from_joint_quorum(const bzn::json_message& joint_quorum)
{
    bzn::json_message single_quorum;
    single_quorum["msg"]["peers"] = joint_quorum["msg"]["peers"]["new"];
    return single_quorum;
}


bzn::json_message
raft::create_joint_quorum_by_removing_peer(const bzn::json_message& last_quorum_message, const bzn::uuid_t& peer_uuid)
{
    bzn::json_message joint_quorum;
    const auto& peers = last_quorum_message["msg"]["peers"];
    joint_quorum["msg"]["peers"]["old"] = peers;

    std::for_each(peers.begin(), peers.end(),
                  [&](const auto& p)
                  {
                      if (p["uuid"]!=peer_uuid)
                      {
                          joint_quorum["msg"]["peers"]["new"].append(p);
                      }
                  });
    return joint_quorum;
}


void
raft::send_session_error_message(std::shared_ptr<bzn::session_base> session, const std::string& error_message)
{
    bzn::json_message response;
    response["error"] = error_message;
    session->send_message(std::make_shared<bzn::json_message>(response), true);
}


bool
raft::validate_new_peer(std::shared_ptr<bzn::session_base> session, const bzn::json_message &peer)
{
    // uuid's are signed as text files, so we must append the end of line character
    const auto uuid{peer["uuid"].asString() + "\x0a"};
    if (!peer.isMember("signature")  || peer["signature"].asString().empty())
    {
        this->send_session_error_message(session, ERROR_INVALID_SIGNATURE);
        return false;
    }

    const auto signature{peer["signature"].asString()};
    if (!bzn::utils::crypto::verify_signature(bzn::utils::crypto::retrieve_bluzelle_public_key_from_contract(), signature, uuid))
    {
        this->send_session_error_message(session, ERROR_UNABLE_TO_VALIDATE_UUID);
        return false;
    }

    if (utils::blacklist::is_blacklisted(this->get_uuid()))
    {
        this->send_session_error_message(session, ERROR_PEER_HAS_BEEN_BLACKLISTED);
        return false;
    }
    return true;
}


void
raft::send_success_message(std::shared_ptr<bzn::session_base> session, const std::string& command, const std::string& message)
{
    bzn::json_message msg;
    msg["bzn-api"] = "raft";
    msg["cmd"] = command;
    msg["response"] = bzn::json_message();
    msg["response"]["from"] = this->get_uuid();
    msg["response"]["msg"] = message;
    session->send_message(std::make_shared<bzn::json_message>(msg), true);
}


void
raft::handle_add_peer(std::shared_ptr<bzn::session_base> session, const bzn::json_message &peer)
{
    if (this->get_state() != bzn::raft_state::leader)
    {
        this->send_session_error_message(session, ERROR_ADD_PEER_MUST_BE_SENT_TO_LEADER);
        return;
    }

    bzn::log_entry last_quorum_entry = this->raft_log->last_quorum_entry();
    if (last_quorum_entry.entry_type == bzn::log_entry_type::joint_quorum)
    {
        this->send_session_error_message(session, MSG_ERROR_CURRENT_QUORUM_IS_JOINT);
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
        this->send_session_error_message(session, ERROR_PEER_ALREADY_EXISTS);
        return;
    }

    if (!peer.isMember("uuid") || peer["uuid"].asString().empty())
    {
        this->send_session_error_message(session, ERROR_INVALID_UUID);
        return;
    }

    if (this->enable_peer_validation && !this->validate_new_peer(session, peer))
    {
        // Note: failure messages sent from within validate_new_peer
        return;
    }

    this->peer_match_index[peer["uuid"].asString()] = 1;
    this->append_log_unsafe(this->create_joint_quorum_by_adding_peer(last_quorum_entry.msg, peer),
                            bzn::log_entry_type::joint_quorum);

    // The add peer has succeded, let's send a message back to the requester
    this->send_success_message(session, "add_peer", SUCCESS_PEER_ADDED_TO_SWARM);
    return;
}


void
raft::handle_remove_peer(std::shared_ptr<bzn::session_base> session, const std::string& uuid)
{
    if (this->get_state() != bzn::raft_state::leader)
    {
        this->send_session_error_message(session, ERROR_REMOVE_PEER_MUST_BE_SENT_TO_LEADER);
        return;
    }

    bzn::log_entry last_quorum_entry = this->raft_log->last_quorum_entry();
    if (last_quorum_entry.entry_type == bzn::log_entry_type::joint_quorum)
    {
        this->send_session_error_message(session, MSG_ERROR_CURRENT_QUORUM_IS_JOINT);
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
        this->send_session_error_message(session, ERROR_PEER_NOT_FOUND);
        return;
    }

    this->append_log_unsafe(this->create_joint_quorum_by_removing_peer(last_quorum_entry.msg, uuid),
                            bzn::log_entry_type::joint_quorum);

    // The remove peer has succeded, let's send a message back to the requester
    this->send_success_message(session, "remove_peer", SUCCESS_PEER_REMOVED_FROM_SWARM);

    return;
}


void
raft::handle_get_peers_response_from_follower_or_candidate(const bzn::json_message& msg)
{
    LOG(debug) << "RAFT - handling get_peers_response from a follower or candidate";
    // one of the following must be true:
    //  2) I'm the follower, here's the leader
    //  3) I'm a candidate, there might be an election happening, try later
    if (msg.isMember("message") && msg["message"].isMember("leader"))
    {
        LOG(debug) << "RAFT - recieved a get_peers response from a follower raft who knows the leader";
        this->leader = msg["message"]["leader"]["uuid"].asString();
        // Since the leader variable gets cleared with each timer expiration,
        // we need to call auto_add_peer_if_required() immediately
        // TODO RHN - pass the leader uuid as a parameter?
        this->auto_add_peer_if_required();
    }
    else
    {
        LOG(debug) << "RAFT - recieved a get_peers_response from a follower, or a candidate who doesn't know the leader - must try again";
    }
    this->in_a_swarm = false;
    return;
}

void
raft::handle_get_peers_response_from_leader(const bzn::json_message& msg)
{
    LOG(debug) << "RAFT - the get_peers_response is from a leader RAFT";
    //  1) I'm the leader, here are the peers
    if (msg.isMember("message"))
    {
        if (msg.isMember("from"))
        {
            this->leader = msg["from"].asString();
            LOG(debug) << "RAFT - setting the leader to [" << this->leader << "]";
        }

        if (msg["message"].isArray())
        {
            auto peer = std::find_if(msg["message"].begin(), msg["message"].end(), [&](const auto& p) { return p["uuid"].asString() == this->get_uuid(); });
            if (peer == msg["message"].end())
            {
                LOG(debug) << "RAFT - I am not in the peers list, need to send add_peer request";
                // we do not find ourselves in the leader's quorum!
                // Time to add oursleves to the swarm!
                this->add_self_to_swarm();
                return;
            }
            LOG(debug) << "RAFT - I am already in the peers list, no further action required";
            // if we get this far, then we know that we are in the
            // leader's quorum and all is good, no need to do anything
            // else, simply remember that we are in a swarm.
            this->in_a_swarm = true;
            return;
        }
        else
        {
            // we do not ever expect a non error response to get_peers to not have an array
            // of peers.
            // TODO: THROW maybe?
            return;
        }
    }
}


void
raft::handle_get_peers_response(const bzn::json_message& msg)
{
    LOG(debug) << "RAFT - recieved get_peers_response";
    // This raft must have sent a get_peers request because it was
    // trying to determine if it's in the swarm
    // There are three options:
    //  1) I'm the leader here are the peers
    //  2) I'm the follower, here's the leader
    //  3) I'm a candidate, there might be an election happening, try later
    if (msg.isMember("error"))
    {
        handle_get_peers_response_from_follower_or_candidate(msg);
        return;
    }
    else
    {
        this->handle_get_peers_response_from_leader(msg);
    }
}


void
raft::handle_ws_raft_messages(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session)
{
    this->shutdown_on_exceeded_max_storage();
    std::lock_guard<std::mutex> lock(this->raft_lock);
    LOG(debug) << "Received WS message:\n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

    // TODO: refactor add/remove peers to move the functionality into handlers
    if (msg["cmd"].asString()=="add_peer")
    {
        // TODO RHN - change cmd for the add_peer response to add_peer_response
        if (msg.isMember("response"))
        {
            if (msg["response"]["msg"].asString() == SUCCESS_PEER_ADDED_TO_SWARM)
            {
                // This is a response to an add peer request that this raft had
                // initialized, this raft is now part of the swarm. Any other
                // response would indicate that the add swarm failed
                this->in_a_swarm = true;
            }
            // TODO RHN we need better responses from add_peer requests.
            return;
        }

        this->handle_add_peer(session, msg["data"]["peer"]);
        return;
    }
    else if (msg["cmd"].asString() == "remove_peer")
    {
        if (!msg.isMember("data") || !msg["data"].isMember("uuid") || msg["data"]["uuid"].asString().empty())
        {
            this->send_session_error_message(session, ERROR_INVALID_UUID);
            return;
        }

        this->handle_remove_peer(session, msg["data"]["uuid"].asString());
        return;
    }
    else if (msg["cmd"].asString() == "get_peers")
    {
        this->handle_get_peers(session);
        return;
    }
    else if ( msg["cmd"].asString() == "get_peers_response")
    {
        this->handle_get_peers_response(msg);
        return;
    }

    // check that the message is from a node in the most recent quorum
    if (!in_quorum(msg["data"]["from"].asString()))
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

                session->send_message(std::make_shared<bzn::json_message>(bzn::create_request_vote_response(this->uuid, this->current_term, true)), true);

                return;
            }

            if (msg["cmd"].asString() == "AppendEntries")
            {
                // TODO: We should either process this message properly, or just drop it after updating term
                this->leader = msg["data"]["from"].asString();

                auto resp_msg = std::make_shared<bzn::json_message>(bzn::create_append_entries_response(this->uuid, this->current_term, false, this->raft_log->size()));

                LOG(debug) << "Sending WS message:\n" << resp_msg->toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

                session->send_message(resp_msg, true);
            }

            LOG(info) << "current term out of sync: " << this->current_term;

            this->update_raft_state(this->current_term, bzn::raft_state::follower);
            this->voted_for.reset();
            this->start_election_timer();
            return;
        }

        // todo: drop back to follower and restart the election?
        if (this->current_term > term)
        {
            LOG(error) << "request had term out of sync: " << this->current_term << " > " << term;

            this->update_raft_state(msg["data"]["term"].asUInt(), bzn::raft_state::follower);
            this->voted_for.reset();
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
    if (!this->enable_audit)
    {
        return;
    }

    audit_message msg;
    msg.mutable_leader_status()->set_term(this->current_term);
    msg.mutable_leader_status()->set_leader(this->uuid);
    msg.mutable_leader_status()->set_current_log_index(this->raft_log->size());
    msg.mutable_leader_status()->set_current_commit_index(this->commit_index);

    auto json_ptr = std::make_shared<bzn::json_message>();
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
    if (!this->enable_audit)
    {
        return;
    }

    audit_message msg;
    msg.mutable_raft_commit()->set_log_index(log_index);
    msg.mutable_raft_commit()->set_operation(operation);

    auto json_ptr = std::make_shared<bzn::json_message>();
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
            bzn::json_message msg;
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

            auto req = std::make_shared<bzn::json_message>(bzn::create_append_entries_request(this->uuid, this->current_term, this->commit_index, prev_index, prev_term, entry_term, msg));

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
raft::handle_request_append_entries_response(const bzn::json_message& msg, std::shared_ptr<bzn::session_base> /*session*/)
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

    uint32_t last_majority_replicated_log_index = this->last_majority_replicated_log_index();
    // TODO: Review the last_majority_replicated_log_index w.r.t. it's bad return values.
    // Intermittently the last_majority_replicated_log_index method returns invalid values
    // that are much larger than the raft log size, this is a hack to stop the leader from
    // crashing.
    if (last_majority_replicated_log_index>this->raft_log->size())
    {
        LOG(error) << "last_majority_replicated_log_index() returned invalid value: [" << last_majority_replicated_log_index << "] - temporarily ignoring entries";
        return;
    }

    while (this->commit_index < last_majority_replicated_log_index)
    {
        this->perform_commit(this->commit_index, this->raft_log->entry_at(this->commit_index));
    }
}


bool
raft::append_log_unsafe(const bzn::json_message& msg, const bzn::log_entry_type entry_type)
{
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
raft::append_log(const bzn::json_message& msg, const bzn::log_entry_type entry_type)
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
raft::get_leader_unsafe()
{
    for(auto& peer : this->get_all_peers())
    {
        if (peer.uuid == this->leader)
        {
            return peer;
        }
    }
    return bzn::peer_address_t("",0,0,"","");
}


bzn::peer_address_t
raft::get_leader()
{
    std::lock_guard<std::mutex> lock(this->raft_lock);
    return this->get_leader_unsafe();
}


void
raft::initialize_storage_from_log(std::shared_ptr<bzn::storage_base> storage)
{
    LOG(info) << "Initializng storage from log entries";
    for (const auto& log_entry : this->raft_log->get_log_entries())
    {
        if (log_entry.entry_type == bzn::log_entry_type::database)
        {
            bzn_msg msg;

            if (!msg.ParseFromString(boost::beast::detail::base64_decode(log_entry.msg["msg"].asString())))
            {
                LOG(error) << "Failed to decode message: " << boost::beast::detail::base64_decode(log_entry.msg["msg"].asString()).substr(0,MAX_MESSAGE_SIZE) << "...";
                continue;
            }

            const bzn::uuid_t uuid = msg.db().header().db_uuid();

            if (msg.db().has_create())
            {
                const database_create& create_command = msg.db().create();
                storage->create(uuid, create_command.key(), create_command.value());
                continue;
            }

            if (msg.db().has_update())
            {
                const database_update& update_command = msg.db().update();
                storage->update(uuid, update_command.key(), update_command.value());
                continue;
            }

            if (msg.db().has_delete_())
            {
                const database_delete& delete_command = msg.db().delete_();
                storage->remove(uuid,delete_command.key());
                continue;
            }
        }
    }
    LOG(info) << "Storage initialized from log entries";
}


std::string
raft::entries_log_path()
{
    // Refactored as the original version was resulting in double '/' occurring
    boost::filesystem::path out{this->state_dir};
    out.append(this->get_uuid()+ ".dat");
    return out.string();
}


void
raft::perform_commit(uint32_t& commit_index, const bzn::log_entry& log_entry)
{
    this->notify_commit(commit_index, log_entry.json_to_string(log_entry.msg));
    this->commit_handler(log_entry.msg);
    commit_index++;

    if (this->get_state() == bzn::raft_state::leader && log_entry.entry_type == bzn::log_entry_type::joint_quorum)
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
                       [&](const auto& uuid)
                       {
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
    // This line will never be reached, but the compiler issues a warning for it anyway
    return std::list<std::set<bzn::uuid_t>>{};
}


bool
raft::is_majority(const std::set<bzn::uuid_t>& votes)
{
    for(const auto& s : this->get_active_quorum())
    {
        std::set<bzn::uuid_t> intersect;
        std::set_intersection(s.begin(), s.end(), votes.begin(), votes.end(),
                              std::inserter(intersect, intersect.begin()));

        if (intersect.size() * 2 <= s.size())
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
    std::vector<std::reference_wrapper<bzn::json_message>> all_peers;
    bzn::log_entry log_entry = this->raft_log->last_quorum_entry();

    if (log_entry.entry_type == bzn::log_entry_type::single_quorum)
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
        for(const bzn::json_message& p : jpeers.get())
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
raft::create_dat_file(const std::string& log_path, const bzn::peers_list_t& peers)
{
    const boost::filesystem::path path(log_path);
    if (!boost::filesystem::exists(path.parent_path()))
    {
        boost::filesystem::create_directories(path.parent_path());
    }

    bzn::json_message root;
    root["msg"] = bzn::json_message();
    for(const auto& p : peers)
    {
        bzn::json_message peer;
        peer["host"] = p.host;
        peer["port"] = p.port;
        peer["http_port"] = p.http_port;
        peer["name"] = p.name;
        peer["uuid"] = p.uuid;
        root["msg"]["peers"].append(peer);
    }

    const bzn::log_entry entry{bzn::log_entry_type::single_quorum, 0, 0, root};

    std::ofstream os( log_path ,std::ios::out | std::ios::binary);
    if (!os)
    {
        throw std::runtime_error(ERROR_UNABLE_TO_CREATE_LOG_FILE_FOR_WRITING + log_path);
    }
    os << entry;
    os.close();
}


void
raft::import_state_files()
{
    const auto& last_entry = this->raft_log->get_log_entries().back();
    if (last_entry.log_index!=this->raft_log->size()-1 || last_entry.term!=this->current_term)
    {
        throw std::runtime_error(MSG_ERROR_INVALID_LOG_ENTRY_FILE);
    }
}


bzn::log_entry_type
raft::deduce_type_from_message(const bzn::json_message& message)
{
    if (message["msg"].isString())
    {
        return bzn::log_entry_type::database;
    }

    if (message["msg"]["peers"].isArray())
    {
        return bzn::log_entry_type::single_quorum;
    }

    if (message["msg"]["peers"].isMember("new"))
    {
        return bzn::log_entry_type::joint_quorum;
    }
    return bzn::log_entry_type::undefined;
}


std::string
raft::get_name()
{
    return "raft";
}


bzn::json_message
raft::get_status()
{
    bzn::json_message status;

    std::lock_guard<std::mutex> lock(this->raft_lock);

    switch(this->current_state)
    {
        case bzn::raft_state::candidate:
            status["state"] = "candidate";
            break;

        case bzn::raft_state::follower:
            status["state"] = "follower";
            break;

        case bzn::raft_state::leader:
            status["state"] = "leader";
            break;

        default:
            status["state"] = "unknown";
            break;
    }

    return status;
}


void
raft::shutdown_on_exceeded_max_storage(bool do_throw)
{
    if (this->raft_log->maximum_storage_exceeded())
    {
        LOG(error) << MSG_ERROR_MAXIMUM_STORAGE_EXCEEDED;
        do_throw ? throw std::runtime_error(MSG_ERROR_MAXIMUM_STORAGE_EXCEEDED) : raise(SIGINT);
    }
}


void
raft::set_audit_enabled(bool val)
{
    this->enable_audit = val;
}


bzn::json_message
raft::to_peer_message(const peer_address_t& address)
{
    bzn::json_message bzn;
    bzn["port"] = address.port;
    bzn["http_port"] = address.http_port;
    bzn["host"] = address.host;
    bzn["uuid"] = address.uuid;
    bzn["name"] = address.name;
    return bzn;
}


void
raft::handle_get_peers(std::shared_ptr<bzn::session_base> session)
{
    bzn::json_message msg;
    msg["bzn-api"] = "raft";
    msg["cmd"] = "get_peers_response";
    msg["from"] = this->get_uuid();

    switch (this->current_state)
    {
        case bzn::raft_state::candidate:
            LOG(error) << ERROR_GET_PEERS_ELECTION_IN_PROGRESS_TRY_LATER;
            msg["error"] = ERROR_GET_PEERS_ELECTION_IN_PROGRESS_TRY_LATER;
            break;
        case bzn::raft_state::follower:
            LOG(error) << ERROR_GET_PEERS_MUST_BE_SENT_TO_LEADER;
            msg["error"] = ERROR_GET_PEERS_MUST_BE_SENT_TO_LEADER;
            msg["message"]["leader"] = this->to_peer_message(this->get_leader_unsafe());
            break;
        case bzn::raft_state::leader:
            for(const auto& peer : this->get_all_peers())
            {
                msg["message"].append(this->to_peer_message(peer));
            }
            break;
        default:
            msg["error"] = ERROR_GET_PEERS_SELECTED_NODE_IN_UNKNOWN_STATE;
            break;
    }
    session->send_message(std::make_shared<bzn::json_message>(msg), false);
}


bzn::peers_list_t
remove_peer_from_peers_list(const bzn::peers_list_t& all_peers, const bzn::uuid_t& uuid_of_peer_to_remove)
{
    bzn::peers_list_t other_peers;
    for(auto& p : all_peers) // TODO: I'd like to try std::copy_if
    {
        if (p.uuid != uuid_of_peer_to_remove)
        {
            other_peers.emplace(p);
        }
    }
    return other_peers;
}


void
raft::auto_add_peer_if_required()
{
    LOG(debug) << "RAFT - may not be in a swarm - finding out";
    // NOTE: if this node is the leader, we don't need to do this test, however, we do this test during startup so
    // there is no way that the node could be a leader...
    // We try any peer that is not ourself, it may come back and say  "i'm not the leader, but try this url",
    // and that would be great, we know the leader so we ask that node for peers in the swarm, hopefully, the leader
    // gives us the list.
    // Or, it may come back and say, there is an election in progress. That's OK, we're just till in limbo.

    // Not that we can't trust our get_all_peers, because we are not the leader, and we may not be in the
    // "official" quorum yet, so we seek out the leader and ask.

    // This may be the second time we've been here, if so that means that we've obtained the
    // uuid of the leader, let's try that

    auto choose_leader=[&]()
            {
                // we don'know who the leader is so we should try someone else
                auto other_peers = remove_peer_from_peers_list(this->get_all_peers(), this->get_uuid());
                if (other_peers.empty())
                {
                    throw std::runtime_error(ERROR_BOOTSTRAP_LIST_MUST_HAVE_MORE_THAN_ONE_PEER);
                }
                return *choose_any_one_of(other_peers);
            };

    bzn::peer_address_t leader{((this->get_leader_unsafe().uuid.empty() && this->get_leader_unsafe().host.empty()) ?  choose_leader() : this->get_leader_unsafe())};

    auto end_point = boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string(leader.host), leader.port};
    bzn::json_message msg;
    msg["bzn-api"] = "raft";
    msg["cmd"] = "get_peers";
    LOG(debug) << "RAFT - sending get_peers to [" << end_point.address().to_string() << ":" << end_point.port() << "]";
    this->node->send_message(end_point, std::make_shared<bzn::json_message>(msg));
}


bzn::json_message
make_add_peer_request(const bzn::peer_address_t& new_peer)
{
    bzn::json_message  msg;
    msg["bzn-api"] = "raft";
    msg["cmd"] = "add_peer";
    msg["data"]["peer"]["name"] = new_peer.name;
    msg["data"]["peer"]["host"] = new_peer.host;
    msg["data"]["peer"]["port"] = new_peer.port;
    msg["data"]["peer"]["http_port"] = new_peer.http_port;
    msg["data"]["peer"]["uuid"] = new_peer.uuid;
    return msg;
}


bzn::json_message
make_secure_add_peer_request(const bzn::peer_address_t& new_peer, const std::string& signed_key)
{
    bzn::json_message  msg{make_add_peer_request(new_peer)};
    msg["data"]["peer"]["signature"] = signed_key;
    return msg;
}


void
raft::add_self_to_swarm()
{
    const auto all_peers = this->get_all_peers();
    const auto this_address = std::find_if(all_peers.begin(), all_peers.end(),
            [&](const auto& peer){return peer.uuid == this->get_uuid();});

    if (this_address == all_peers.end())
    {
        // If peer validation was enabled, it could be that the daemons' uuid
        // was black listed, in that case bail quickly.
        if (this->enable_peer_validation && bzn::utils::blacklist::is_blacklisted(this->get_uuid()))
        {
            // we handle runtime errors before they reach our
            // catch(std::exception) block, so tell the user what
            // the problem is now.
            LOG(fatal) << ERROR_PEER_BLACKLISTED;
            throw std::runtime_error(ERROR_PEER_BLACKLISTED);
        }

        // It could also be that the owner simply made a mistake in the
        // peers.json file, or forgot to add the peer info
        LOG(fatal) << ERROR_UNABLE_TO_ADD_PEER_TO_SWARM;
        throw std::runtime_error(ERROR_UNABLE_TO_ADD_PEER_TO_SWARM);
    }

    bzn::json_message request{make_secure_add_peer_request(*this_address, this->signed_key)};

    bzn::peer_address_t leader{this->get_leader_unsafe()};
    auto end_point = boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string(leader.host), leader.port};

    LOG(debug) << "RAFT sending add_peer command to the leader:\n" << request.toStyledString();



    this->node->send_message(end_point, std::make_shared<bzn::json_message>(request));
}

