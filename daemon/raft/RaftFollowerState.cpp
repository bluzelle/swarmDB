#include <iostream>

#include <boost/asio/placeholders.hpp>

#include "PeerList.h"
#include "CommandFactory.h"
#include "RaftFollowerState.h"
#include "JsonTools.h"

RaftFollowerState::RaftFollowerState(boost::asio::io_service& ios,
                                     Storage& s,
                                     CommandFactory& cf,
                                     ApiCommandQueue& pq,
                                     PeerList& ps,
                                     function<string(const string&)> rh,
                                     unique_ptr<RaftState>& ns)
        : RaftState(ios, s, cf, pq, ps, rh, ns),
          heartbeat_timer_(ios_,
                           boost::posix_time::milliseconds(RaftState::raft_election_timeout_interval_min_milliseconds))
{
    std::cout << "          I am Follower" << std::endl;

    heartbeat_timer_.async_wait(boost::bind(&RaftFollowerState::heartbeat_timer_expired,
                                            this, boost::asio::placeholders::error));
}

RaftFollowerState::~RaftFollowerState()
{
    heartbeat_timer_.cancel();
}

unique_ptr<RaftState> RaftFollowerState::handle_request(const string& request, string& response)
{
    auto pt = pt_from_json_string(request);

    unique_ptr<Command> command = command_factory_.get_command(pt, *this);
    if (command != nullptr)
        response = pt_to_json_string(command->operator()());

    return nullptr;
}


void RaftFollowerState::heartbeat_timer_expired(const boost::system::error_code& e)
{
    if (e != boost::asio::error::operation_aborted)
        {
        std::cout << "Starting Leader election" << std::endl;

        set_next_state_candidate();
        }
}

void RaftFollowerState::rearm_heartbeat_timer() // Use with flag to stop it when transitioned to another state
{
    heartbeat_timer_.cancel();
    heartbeat_timer_.expires_from_now(
            boost::posix_time::milliseconds(RaftState::raft_election_timeout_interval_min_milliseconds));

    heartbeat_timer_.async_wait(boost::bind(&RaftFollowerState::heartbeat_timer_expired,
                                            this, boost::asio::placeholders::error));
}
