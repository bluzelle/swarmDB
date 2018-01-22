#include <iostream>
#include <utility>
#include <thread>
#include <sstream>
#include <utility>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "Raft.h"
#include "JsonTools.h"
#include "DaemonInfo.h"
#include "RaftCandidateState.h"
#include "RaftLeaderState.h"
#include "RaftFollowerState.h"

static boost::uuids::uuid s_transaction_id;

DaemonInfo& daemon_info = DaemonInfo::get_instance();

Raft::Raft(boost::asio::io_service &ios)
        : ios_(ios),
          peers_(ios_),
          storage_("./storage_" + daemon_info.host_name() + ".txt"),
          command_factory_(
                  storage_,
                  peer_queue_)
{
    static boost::uuids::nil_generator nil_uuid_gen;
    s_transaction_id = nil_uuid_gen();
}

void Raft::run()
{
    raft_state_ = std::make_unique<RaftCandidateState>(ios_,
                                                       storage_,
                                                       command_factory_,
                                                       peer_queue_,
                                                       peers_,
                                                       std::bind(&Raft::handle_request,
                                                              this,
                                                              std::placeholders::_1),
                                                       raft_next_state_);
}

string Raft::handle_request(const string &req)
{
    // State transition can be a result of timer (Follower->Candidate) or command execution.
    // First check if there is a pending state transition caused by timer:
    {
        std::lock_guard<mutex> lock(raft_next_state_mutex_);
        if (raft_next_state_ != nullptr)
            {
            std::lock_guard<mutex> lock(raft_state_mutex_);
            raft_state_.reset(raft_next_state_.release()); // Current state becomes next state.
            }
    }


    std::lock_guard<mutex> lock(raft_state_mutex_); // Requests come from multiple peers. Handle one at a time. Potential bottleneck.

    string resp;
    unique_ptr<RaftState> next_state = raft_state_->handle_request(req, resp); // request handled by current state.

    // Check if there is state transtion followed by command execution.
    if (next_state != nullptr)
        raft_state_.reset(next_state.get());

    return resp;
}




