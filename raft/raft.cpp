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

namespace
{
    const std::string NO_PEERS_ERRORS_MGS = "No peers given!";
    const std::string MSG_ERROR_INVALID_LOG_ENTRY_FILE{"Invalid log entry file. Please Delete .state folder."};
    const std::string MSG_ERROR_EMPTY_LOG_ENTRY_FILE{"Empty log entry file. Please delete .state folder."};
    const std::string MSG_ERROR_INVALID_STATE_FILE{"Invalid state file. Please delete the .state folder."};
    const std::string MSG_NO_PEERS_IN_LOG = "Unable to find peers in log entries.";
    const std::chrono::milliseconds DEFAULT_HEARTBEAT_TIMER_LEN{std::chrono::milliseconds(1000)};
    const std::chrono::milliseconds  DEFAULT_ELECTION_TIMER_LEN{std::chrono::milliseconds(5000)};

    const std::string RAFT_TIMEOUT_SCALE = "RAFT_TIMEOUT_SCALE";
}


using namespace bzn;


raft::raft(
        std::shared_ptr<bzn::asio::io_context_base> io_context
        , std::shared_ptr<bzn::node_base> node
        , const bzn::peers_list_t& peers
        , bzn::uuid_t uuid)
    : timer(io_context->make_unique_steady_timer())
    , uuid(std::move(uuid))
    , node(std::move(node))
{
    // we must have a list of peers!
    if (peers.empty())
    {
        throw std::runtime_error(NO_PEERS_ERRORS_MGS);
    }
    this->setup_peer_tracking(peers);
    this->get_raft_timeout_scale();

    this->state_files_exist() ? this->import_state_files()
                               : this->create_state_files(peers);
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

            this->node->register_for_message("raft", std::bind(&raft::handle_ws_raft_messages, shared_from_this(),
                std::placeholders::_1, std::placeholders::_2));
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

            this->node->send_message(ep, std::make_shared<bzn::message>(bzn::create_request_vote_request(this->uuid, this->current_term, this->last_log_index, this->last_log_term)));
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

    bool vote = msg["data"]["lastLogIndex"].asUInt() >= this->last_log_index;

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

    // if our log entry is empty...
    if (this->log_entries.empty())
    {
        success = true;

        if (!msg["data"]["entries"].empty() && leader_prev_index == 0)
        {
            this->log_entries.emplace_back(
                log_entry{bzn::log_entry_type::log_entry, ++this->last_log_index, entry_term, msg["data"]["entries"]});
        }
    }
    else
    {
        if (this->log_entries.back().log_index == leader_prev_index &&
            this->log_entries.back().term == leader_prev_term)
        {
            success = true;

            if (!msg["data"]["entries"].empty())
            {
                this->log_entries.emplace_back(
                    log_entry{bzn::log_entry_type::log_entry, ++this->last_log_index, entry_term, msg["data"]["entries"]});
            }
        }
        else
        {
            // todo: we are out of sync with leader so lets walk back and pop the prev index?
            if (msg["data"]["commitIndex"].asUInt() < this->last_log_index)
            {
                if (this->commit_index < this->last_log_index)
                {
                    this->log_entries.pop_back();
                    --this->last_log_index;
                }
                else
                {
                    LOG(debug) << "at commit index: " << this->commit_index;
                }
            }
        }
    }

    auto resp_msg = std::make_shared<bzn::message>(bzn::create_append_entries_response(this->uuid, this->current_term, success, this->last_log_index));

    LOG(debug) << "Sending WS message:\n" << resp_msg->toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

    session->send_message(resp_msg, true);

    // update commit index...
    if (success)
    {
        if (this->commit_index < msg["data"]["commitIndex"].asUInt())
        {
            for(size_t i = this->commit_index; i < std::min(this->log_entries.size(), size_t(msg["data"]["commitIndex"].asUInt())); ++i)
            {
                this->perform_commit(commit_index, this->log_entries[i]);
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
    joint_quorum["old"] = joint_quorum["new"] = last_quorum_message;
    joint_quorum["new"].append(new_peer);
    return joint_quorum;
}


bzn::message
raft::create_single_quorum_from_joint_quorum(const bzn::message& joint_quorum)
{
    return joint_quorum["new"];
}


bzn::message
raft::create_joint_quorum_by_removing_peer(const bzn::message &last_quorum_message, const bzn::uuid_t& peer_uuid)
{
    bzn::message joint_quorum;
    joint_quorum["old"] = last_quorum_message;
    std::for_each(last_quorum_message.begin(), last_quorum_message.end(),
                  [&](const auto& p)
                  {
                      if(p["uuid"]!=peer_uuid)
                      {
                          joint_quorum["new"].append(p);
                      }
                  });
    return joint_quorum;
}


void
raft::handle_ws_raft_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
{
    std::lock_guard<std::mutex> lock(this->raft_lock);
    LOG(debug) << "Received WS message:\n" << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

    // TODO: refactor add/remove peers to move the functionality into handlers
    if(msg["cmd"].asString()=="add_peer")
    {
        if(this->get_state() != bzn::raft_state::leader)
        {
            bzn::message response;
            response["error"] = ERROR_ADD_PEER_MUST_BE_SENT_TO_LEADER;
            session->send_message(std::make_shared<bzn::message>(response), true);
            return;
        }

        bzn::log_entry last_quorum_entry = this->last_quorum();
        if(last_quorum_entry.entry_type == bzn::log_entry_type::joint_quorum)
        {
            bzn::message response;
            response["error"] = MSG_ERROR_CURRENT_QUORUM_IS_JOINT;
            session->send_message(std::make_shared<bzn::message>(response), true);
            return;
        }

        const auto& same_peer = std::find_if(last_quorum_entry.msg.begin(), last_quorum_entry.msg.end(),
                [&](const auto& p)
                {
                    return p["uuid"].asString() == msg["data"]["peer"]["uuid"].asString();
                });
        if(same_peer != last_quorum_entry.msg.end())
        {
            bzn::message response;
            response["error"] = ERROR_PEER_ALREADY_EXISTS;
            session->send_message(std::make_shared<bzn::message>(response), true);
            return;
        }

        this->peer_match_index[msg["data"]["peer"]["uuid"].asString()] = 1;
        this->append_log_unsafe(this->create_joint_quorum_by_adding_peer(last_quorum_entry.msg, msg["data"]["peer"]), bzn::log_entry_type::joint_quorum);
        return;
    }
    else if(msg["cmd"].asString() == "remove_peer" )
    {
        if(this->get_state() != bzn::raft_state::leader)
        {
            bzn::message response;
            response["error"] = ERROR_REMOVE_PEER_MUST_BE_SENT_TO_LEADER;
            session->send_message(std::make_shared<bzn::message>(response), true);
            return;
        }

        bzn::log_entry last_quorum_entry = this->last_quorum();
        if(last_quorum_entry.entry_type == bzn::log_entry_type::joint_quorum)
        {
            bzn::message response;
            response["error"] = MSG_ERROR_CURRENT_QUORUM_IS_JOINT;
            session->send_message(std::make_shared<bzn::message>(response), true);
            return;
        }

        const auto& same_peer = std::find_if(last_quorum_entry.msg.begin(), last_quorum_entry.msg.end(),
                                             [&](const auto& p)
                                             {
                                                 return p["uuid"].asString() == msg["data"]["uuid"].asString();
                                             });
        if(same_peer == last_quorum_entry.msg.end())
        {
            bzn::message response;
            response["error"] = ERROR_PEER_NOT_FOUND;
            session->send_message(std::make_shared<bzn::message>(response), true);
            return;
        }

        this->append_log_unsafe(this->create_joint_quorum_by_removing_peer(last_quorum_entry.msg, msg["data"]["uuid"].asString()), bzn::log_entry_type::joint_quorum);
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
                this->leader = msg["data"]["from"].asString();

                auto resp_msg = std::make_shared<bzn::message>(bzn::create_append_entries_response(this->uuid, this->current_term, false, this->last_log_index));

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

            if (this->peer_match_index[peer.uuid] == 0 && !this->log_entries.empty())
            {
                msg = this->log_entries.front().msg;
                entry_term = this->log_entries.front().term;
            }
            else
            {
                for (const log_entry& entry : boost::adaptors::reverse(this->log_entries))
                {
                    if (entry.log_index == this->peer_match_index[peer.uuid])
                    {
                        prev_index = entry.log_index;
                        prev_term  = entry.term;

                        if (entry.log_index < this->log_entries.size())
                        {
                            msg = this->log_entries[entry.log_index].msg;
                            entry_term = this->log_entries[entry.log_index].term;
                        }
                        break;
                    }
                }
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
    if (msg["data"]["matchIndex"].asUInt() > this->log_entries.size() ||
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

    uint32_t consensus_match_index = this->last_majority_replicated_log_index();
    while (this->commit_index < consensus_match_index)
    {
        this->perform_commit(this->commit_index, this->log_entries[this->commit_index]);
    }


    // if the last currnt quorum is a joint quorum then create a single quorum and add it to the log




}


bool
raft::append_log_unsafe(const bzn::message& msg, const bzn::log_entry_type entry_type)
{
    if (this->current_state != bzn::raft_state::leader)
    {
        LOG(warning) << "not the leader, can't append log_entries!";
        return false;
    }

    this->log_entries.emplace_back(log_entry{entry_type, ++this->last_log_index, this->current_term, msg});

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
    for (const auto& log_entry : this->log_entries)
    {
        if(log_entry.entry_type == bzn::log_entry_type::log_entry)
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
    return "./.state/" + this->get_uuid() +".dat";
}


void
raft::append_entry_to_log(const bzn::log_entry& log_entry)
{
    if (!this->log_entry_out_stream.is_open())
    {
        boost::filesystem::path path{this->entries_log_path()};
        if(!boost::filesystem::exists(path.parent_path()))
        {
            boost::filesystem::create_directories(path.parent_path());
        }
        this->log_entry_out_stream.open(path.string(), std::ios::out |  std::ios::binary | std::ios::app);
    }
    this->log_entry_out_stream << log_entry;
    this->log_entry_out_stream.flush();
}


std::string
raft::state_path()
{
    return "./.state/" + this->get_uuid() +".state";
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
    os << this->last_log_index  << " " << this->last_log_term << " " << this->commit_index << " " <<  this->current_term;
    os.close();
}


void
raft::load_state()
{
    this->last_log_index = std::numeric_limits<uint32_t>::max();
    this->last_log_term = std::numeric_limits<uint32_t>::max();
    this->commit_index = std::numeric_limits<uint32_t>::max();
    this->current_term = std::numeric_limits<uint32_t>::max();
    std::ifstream is(this->state_path(), std::ios::in | std::ios::binary);
    is >> this->last_log_index >> this->last_log_term >> this->commit_index >> this->current_term;
    if (std::numeric_limits<uint32_t>::max()==this->last_log_index
        || std::numeric_limits<uint32_t>::max()==this->last_log_term
        || std::numeric_limits<uint32_t>::max()==this->commit_index
        || std::numeric_limits<uint32_t>::max()==this->current_term)
    {
        throw std::runtime_error(MSG_ERROR_INVALID_STATE_FILE);
    }
    is.close();
}


void
raft::load_log_entries()
{
    std::ifstream is(this->entries_log_path(), std::ios::in | std::ios::binary);
    bzn::log_entry log_entry;
    while (is >> log_entry)
    {
        this->log_entries.emplace_back(log_entry);
    }
    is.close();

    if (this->log_entries.empty())
    {
        throw std::runtime_error(MSG_ERROR_EMPTY_LOG_ENTRY_FILE);
    }
}


void
raft::perform_commit(uint32_t& commit_index, const bzn::log_entry& log_entry)
{
    this->commit_handler(log_entry.msg);
    this->append_entry_to_log(log_entry);
    commit_index++;
    this->save_state();

    if(this->get_state() == bzn::raft_state::leader && log_entry.entry_type == bzn::log_entry_type::joint_quorum)
    {
        this->append_log_unsafe(
                this->create_single_quorum_from_joint_quorum(log_entry.msg),
                bzn::log_entry_type::single_quorum);
    }
}


bzn::log_entry
raft::last_quorum()
{
    // TODO: Speed this up by not doing a search, when a quorum entry is added, simply store the index. Perhaps only do the search if the index is wrong.
    auto result = std::find_if(
            this->log_entries.crbegin(),
            this->log_entries.crend(),
            [](const auto& entry)
            {
                return entry.entry_type != bzn::log_entry_type::log_entry;
            });
    if(result == this->log_entries.crend())
    {
        throw std::runtime_error(MSG_NO_PEERS_IN_LOG);
    }
    return *result;
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

    bzn::log_entry log_entry = this->last_quorum();
    switch(log_entry.entry_type) {
        case bzn::log_entry_type::single_quorum: {
            std::set<bzn::uuid_t> result;
            std::transform(log_entry.msg.begin(), log_entry.msg.end(), std::inserter(result, result.begin()),
                           extract_uuid);

            return std::list<std::set<bzn::uuid_t>>{result};
        }
        case bzn::log_entry_type::joint_quorum: {
            std::set<bzn::uuid_t> result_old, result_new;
            std::transform(log_entry.msg["old"].begin(), log_entry.msg["old"].end(),
                           std::inserter(result_old, result_old.begin()),
                           extract_uuid);
            std::transform(log_entry.msg["new"].begin(), log_entry.msg["new"].end(),
                           std::inserter(result_new, result_new.begin()),
                           extract_uuid);

            return std::list<std::set<bzn::uuid_t>>{result_old, result_new};
        }
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
    bzn::log_entry log_entry = this->last_quorum();

    if(log_entry.entry_type == bzn::log_entry_type::single_quorum)
    {
        all_peers.emplace_back(log_entry.msg);
    }
    else
    {
        all_peers.emplace_back(log_entry.msg["old"]);
        all_peers.emplace_back(log_entry.msg["new"]);
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
    const auto quorums = this->get_active_quorum();
    for(const auto q : quorums)
    {
        if (std::find(q.begin(), q.end(),uuid) != q.end())
        {
            return true;
        }
    }
    return false;
}


void
raft::create_state_files(const bzn::peers_list_t& peers)
{
    bzn::message root;
    for(const auto& p : peers)
    {
        bzn::message peer;
        peer["host"] = p.host;
        peer["port"] = p.port;
        peer["http_port"] = p.http_port;
        peer["name"] = p.name;
        peer["uuid"] = p.uuid;
        root.append(peer);
    }
    const bzn::log_entry entry{
            bzn::log_entry_type::single_quorum,
            0,
            0,
            root};

    this->log_entries.emplace_back(entry);
    this->append_entry_to_log(entry);
    this->save_state();
}

void
raft::import_state_files()
{
    this->load_state();
    this->load_log_entries();
    const auto& last_entry = this->log_entries.back();
    if (last_entry.log_index!=this->last_log_index || last_entry.term!=this->current_term)
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
