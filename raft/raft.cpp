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

namespace
{
    const std::string NO_PEERS_ERRORS_MGS = " No peers given!";

    const std::chrono::milliseconds DEFAULT_HEARTBEAT_TIMER_LEN{std::chrono::milliseconds(1000)};
    const std::chrono::milliseconds  DEFAULT_ELECTION_TIMER_LEN{std::chrono::milliseconds(5000)};
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
    std::uniform_int_distribution<uint32_t> dist(DEFAULT_HEARTBEAT_TIMER_LEN.count() * 3 , DEFAULT_ELECTION_TIMER_LEN.count());

    auto timeout = std::chrono::milliseconds(dist(gen));

    LOG(info) << "election timer will expire in: " << timeout.count() << "ms";

    this->timer->expires_from_now(timeout);

    this->timer->async_wait(std::bind(&raft::handle_election_timeout, shared_from_this(), std::placeholders::_1));
}


void
raft::start_heartbeat_timer()
{
    this->timer->cancel();

    LOG(debug) << "heartbeat timer will expire in: " << DEFAULT_HEARTBEAT_TIMER_LEN.count() << "ms";

    this->timer->expires_from_now(DEFAULT_HEARTBEAT_TIMER_LEN);

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

            this->node->send_message(ep, bzn::create_request_vote_request(this->uuid, this->current_term, this->last_log_index, this->last_log_term),
                [peer, self = shared_from_this()](const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
                {
                    if (!session)
                    {
                        LOG(warning) << "did not receive a RequestVote response from peer: " << peer.name
                             << " (" << peer.host << ":" << peer.port << ")";

                        return;
                    }

                    self->handle_request_vote_response(msg, std::move(session));
                });
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
    LOG(debug) << '\n' << msg.toStyledString();

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
        session->send_message(bzn::create_request_vote_response(this->uuid, this->current_term, false), nullptr);

        return;
    }

    // vote for this peer...
    this->voted_for = msg["data"]["from"].asString();

    bool vote = msg["data"]["lastLogIndex"].asUInt() >= this->last_log_index;

    session->send_message(bzn::create_request_vote_response(this->uuid, this->current_term, vote), nullptr);
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
                log_entry{++this->last_log_index, entry_term, msg["data"]["entries"]});
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
                    log_entry{++this->last_log_index, entry_term, msg["data"]["entries"]});
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

    auto resp_msg = bzn::create_append_entries_response(this->uuid, this->current_term, success, this->last_log_index);

    LOG(debug) << "Sending WS message:\n" << resp_msg.toStyledString();

    session->send_message(resp_msg, nullptr);

    // update commit index...
    if (success)
    {
        if (this->commit_index < msg["data"]["commitIndex"].asUInt())
        {
            for(size_t i = this->commit_index; i < std::min(this->log_entries.size(), size_t(msg["data"]["commitIndex"].asUInt())); ++i)
            {
                this->commit_handler(this->log_entries[i].msg);
                this->commit_index++;
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
    LOG(debug) << "Received WS message:\n" << msg.toStyledString();

    uint32_t term = msg["data"]["term"].asUInt();

    if (this->current_term == term)
    {
        if (msg["cmd"].asString() == "RequestVote")
        {
            this->handle_ws_request_vote(msg, session);
            return;
        }

        if (msg["cmd"].asString() == "AppendEntries")
        {
            this->handle_ws_append_entries(msg, session);
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

                session->send_message(bzn::create_request_vote_response(this->uuid, this->current_term, true), nullptr);

                return;
            }

            if (msg["cmd"].asString() == "AppendEntries")
            {
                this->leader = msg["data"]["from"].asString();

                auto resp_msg = bzn::create_append_entries_response(this->uuid, this->current_term, true, this->last_log_index);

                LOG(debug) << "Sending WS message:\n" << resp_msg.toStyledString();

                session->send_message(resp_msg, nullptr);
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

            auto req = bzn::create_append_entries_request(this->uuid, this->current_term, this->commit_index, prev_index, prev_term, entry_term, msg);

            LOG(debug) << "Sending request:\n" << req.toStyledString();

            this->node->send_message(ep, req,
                [peer, self = shared_from_this()](const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
                {
                    if (!session)
                    {
                        LOG(warning) << "did not receive a AppendEntries response from: "
                                     << peer.name << " (" << peer.host << ":" << peer.port << ")";

                        return;
                    }

                    self->handle_request_append_entries_response(msg, std::move(session));
                });
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

    LOG(debug) << "Received AppendEntries response:\n" << msg.toStyledString();

    // check match index for bad peers...
    if (msg["data"]["matchIndex"].asUInt() > this->log_entries.size() ||
        msg["data"]["term"].asUInt() != this->current_term)
    {
        LOG(error) << "received bad match index or term: \n" << msg.toStyledString();
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
        this->commit_handler(this->log_entries[this->commit_index++].msg);
    }
}


bool
raft::append_log(const bzn::message& msg)
{
    if (this->current_state != bzn::raft_state::leader)
    {
        LOG(warning) << "not the leader can't append log_entries!";
        return false;
    }

    this->log_entries.emplace_back(raft::log_entry{++this->last_log_index, this->current_term, msg});

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
            LOG(info) << "\n"
                         " ___                     _                   _           \n"
                         "|_ _|   __ _ _ __ ___   | |    ___  __ _  __| | ___ _ __ \n"
                         " | |   / _` | '_ ` _ \\  | |   / _ \\/ _` |/ _` |/ _ \\ '__|\n"
                         " | |  | (_| | | | | | | | |__|  __/ (_| | (_| |  __/ |   \n"
                         "|___|  \\__,_|_| |_| |_| |_____\\___|\\__,_|\\__,_|\\___|_|\n";

            this->leader = this->uuid;
            break;

        case bzn::raft_state::candidate:
            LOG(info) << "\n"
                         " ___                      ____                _ _     _       _       \n"
                         "|_ _|   __ _ _ __ ___    / ___|__ _ _ __   __| (_) __| | __ _| |_ ___ \n"
                         " | |   / _` | '_ ` _ \\  | |   / _` | '_ \\ / _` | |/ _` |/ _` | __/ _ \\\n"
                         " | |  | (_| | | | | | | | |__| (_| | | | | (_| | | (_| | (_| | ||  __/\n"
                         "|___|  \\__,_|_| |_| |_|  \\____\\__,_|_| |_|\\__,_|_|\\__,_|\\__,_|\\__\\___|\n";
            break;

        case bzn::raft_state::follower:
            LOG(info) << "\n"
                         " ___                     _____     _ _                        \n"
                         "|_ _|   __ _ _ __ ___   |  ___|__ | | | _____      _____ _ __ \n"
                         " | |   / _` | '_ ` _ \\  | |_ / _ \\| | |/ _ \\ \\ /\\ / / _ \\ '__|\n"
                         " | |  | (_| | | | | | | |  _| (_) | | | (_) \\ V  V /  __/ |   \n"
                         "|___|  \\__,_|_| |_| |_| |_|  \\___/|_|_|\\___/ \\_/\\_/ \\___|_|  \n";
            break;

        default:
            break;
    }
}


bzn::peer_address_t
raft::get_leader()
{
    for(auto& peer : this->peers)
    {
        if (peer.uuid == this->leader)
        {
            return peer;
        }
    }

    return bzn::peer_address_t("",0,"","");
}
