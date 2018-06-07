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

    const std::chrono::milliseconds DEFAULT_HEARTBEAT_TIMER_LEN{std::chrono::milliseconds(1000)};
    const std::chrono::milliseconds  DEFAULT_ELECTION_TIMER_LEN{std::chrono::milliseconds(5000)};

    const std::string RAFT_TIMEOUT_SCALE = "RAFT_TIMEOUT_SCALE";
}


using namespace bzn;


raft::raft(std::shared_ptr<bzn::asio::io_context_base> io_context, std::shared_ptr<bzn::node_base> node, const bzn::peers_list_t& peers, bzn::uuid_t uuid)
    : timer(io_context->make_unique_steady_timer())
    , peers(peers)
    , uuid(std::move(uuid))
    , node(std::move(node))
{
    // we must have a list of peers!
    if (this->peers.empty())
    {
        throw std::runtime_error(NO_PEERS_ERRORS_MGS);
    }

    // setup peer tracking...
    for (const auto& peer : this->peers)
    {
        this->peer_match_index[peer.uuid] = 0;
    }

    this->get_raft_timeout_scale();

    if (boost::filesystem::exists(this->state_path()) && boost::filesystem::exists(this->entries_log_path()))
    {
        this->load_state();
        this->load_log_entries();
    }
    else
    {
        bzn::message root;
        for(const auto& p : this->peers)
        {
            bzn::message peer;
            peer["host"] = p.host;
            peer["port"] = p.port;
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
        const auto& last_entry = this->log_entries.back();
        if (last_entry.log_index!=this->last_log_index || last_entry.term!=this->current_term)
        {
            throw std::runtime_error(MSG_ERROR_INVALID_LOG_ENTRY_FILE);
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
    this->yes_votes = 1;
    this->no_votes = 0;
    ++this->current_term;
    this->update_raft_state(this->current_term, bzn::raft_state::candidate);
    this->leader.clear();

    for (const auto& peer : this->peers)
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
    LOG(debug) << '\n' << msg.toStyledString().substr(0, 60) << "...";

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
        if (++this->yes_votes > this->peers.size()/2)
        {
            this->update_raft_state(this->current_term, bzn::raft_state::leader);

            // clear any previous peer state...
            for (auto& entry : this->peer_match_index)
            {
                entry.second = 0;
            }

            this->request_append_entries();

            return;
        }
    }
    else
    {
        if (++this->no_votes > this->peers.size()/2)
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

    LOG(debug) << "Sending WS message:\n" << resp_msg->toStyledString().substr(0, 60) << "...";

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


void
raft::handle_ws_raft_messages(const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
{
    std::lock_guard<std::mutex> lock(this->raft_lock);

    LOG(debug) << "Received WS message:\n" << msg.toStyledString().substr(0, 60) << "...";

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

                auto resp_msg = std::make_shared<bzn::message>(bzn::create_append_entries_response(this->uuid, this->current_term, true, this->last_log_index));

                LOG(debug) << "Sending WS message:\n" << resp_msg->toStyledString().substr(0, 60) << "...";

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
    for (const auto& peer : this->peers)
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

            LOG(debug) << "Sending request:\n" << req->toStyledString().substr(0, 60) << "...";

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
        LOG(error) << "received bad match index or term: \n" << msg.toStyledString().substr(0, 60) << "...";
        return;
    }

    this->peer_match_index[msg["data"]["from"].asString()] = msg["data"]["matchIndex"].asUInt();

    if (!msg["data"]["success"].asBool())
    {
        LOG(debug) << "append entry failed for peer: " << msg["data"]["uuid"].asString();
        return;
    }

    // copy over match_log_index
    std::vector<uint32_t> match_indexes;
    for(const auto& match_index : this->peer_match_index)
    {
        match_indexes.push_back(match_index.second);
    }
    std::sort(match_indexes.begin(), match_indexes.end());
    size_t consensus_commit_index = match_indexes[uint32_t(std::ceil(match_indexes.size()/2.0))];

    while (this->commit_index < consensus_commit_index)
    {
        this->perform_commit(this->commit_index, this->log_entries[this->commit_index]);
    }
}


bool
raft::append_log(const bzn::message& msg)
{
    std::lock_guard<std::mutex> lock(this->raft_lock);

    if (this->current_state != bzn::raft_state::leader)
    {
        LOG(warning) << "not the leader, can't append log_entries!";
        return false;
    }

    this->log_entries.emplace_back(log_entry{bzn::log_entry_type::log_entry, ++this->last_log_index, this->current_term, msg});

    return true;
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

    for(auto& peer : this->peers)
    {
        if (peer.uuid == this->leader)
        {
            return peer;
        }
    }

    return bzn::peer_address_t("",0,"","");
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
}


bzn::log_entry
raft::last_quorum()
{
    // TODO: Speed this up by not doing a search, when a quorum entry is added, simply store the index. Perhaps only do the search if the index is wrong.
    auto it = this->log_entries.end();
    while(it != this->log_entries.begin())
    {
        --it;
        if  (it->entry_type != bzn::log_entry_type::log_entry)
        {
            break;
        }
    }
    // assumes that the first entry in the log is always a quorum type.
    return *it;
}

