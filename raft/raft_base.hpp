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

#pragma once

#include <bootstrap/peer_address.hpp>
#include <raft/log_entry.hpp>

namespace bzn
{
    enum class raft_state : uint8_t
    {
        candidate=0,
        follower,
        leader
    };

    ///////////////////////////////////////////////////////////////////////////
    // todo: Use protobufs!

    inline bzn::message
    create_request_vote_request(const bzn::uuid_t& uuid, uint32_t current_term, uint32_t last_log_index, uint32_t last_log_term)
    {
        bzn::message msg;

        msg["bzn-api"] = "raft";
        msg["cmd"] = "RequestVote";
        msg["data"] = bzn::message();
        msg["data"]["from"] = uuid;
        msg["data"]["term"] = current_term;
        msg["data"]["lastLogIndex"] = last_log_index;
        msg["data"]["lastLogTerm"] = last_log_term;

        return msg;
    }


    inline bzn::message
    create_request_vote_response(const bzn::uuid_t& uuid, uint32_t current_term, bool granted)
    {
        bzn::message msg;

        msg["bzn-api"] = "raft";
        msg["cmd"] = "ResponseVote";
        msg["data"] = bzn::message();
        msg["data"]["from"] = uuid;
        msg["data"]["term"] = current_term;
        msg["data"]["granted"] = granted;
        msg["data"]["lastLogIndex"] = 0;
        msg["data"]["lastLogTerm"] = 0;

        return msg;
    }


    inline bzn::message
    create_append_entries_request(const bzn::uuid_t& uuid, uint32_t current_term, uint32_t commit_index, uint32_t prev_index,
        uint32_t prev_term, uint32_t entry_term, bzn::message entry)
    {
        bzn::message msg;

        msg["bzn-api"] = "raft";
        msg["cmd"] = "AppendEntries";
        msg["data"] = bzn::message();
        msg["data"]["from"] = uuid;
        msg["data"]["term"] = current_term;
        msg["data"]["prevIndex"] = prev_index;
        msg["data"]["prevTerm"] = prev_term;
        msg["data"]["entries"] = entry;
        msg["data"]["entryTerm"] = entry_term;
        msg["data"]["commitIndex"] = commit_index;

        return msg;
    }


    inline bzn::message
    create_append_entries_response(const bzn::uuid_t& uuid, uint32_t current_term, bool success, uint32_t match_index)
    {
        bzn::message msg;

        msg["bzn-api"] = "raft";
        msg["cmd"] = "AppendEntriesReply";
        msg["data"] = bzn::message();
        msg["data"]["from"] = uuid;
        msg["data"]["term"] = current_term;
        msg["data"]["success"] = success;
        msg["data"]["matchIndex"] = match_index;

        return msg;
    }


    class raft_base
    {
    public:
        using commit_handler = std::function<bool(const bzn::message& msg)>;

        virtual ~raft_base() = default;

        /**
         * Return the current raft state
         * @return raft_state
         */
        virtual bzn::raft_state get_state() = 0;

        /**
         * Start raft
         */
        virtual void start() = 0;

        /**
         * Current leader
         * @return uuid of leader or blank if unknown
         */
        virtual bzn::peer_address_t get_leader() = 0;

        /**
         * Appends entry to leader's log via CRUD
         * @param msg message received
         */
        virtual bool append_log(const bzn::message& msg, bzn::log_entry_type entry_type) = 0;

        /**
         * Storage commit handler called once concensus has been achieved
         * @param handler callback
         */
        virtual void register_commit_handler(bzn::raft_base::commit_handler handler) = 0;

    };


} // namespace bzn
