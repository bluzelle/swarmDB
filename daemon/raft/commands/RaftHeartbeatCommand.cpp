#include "RaftFollowerState.h"
#include "RaftCandidateState.h"

#include "RaftHeartbeatCommand.h"

#include "JsonTools.h"


RaftHeartbeatCommand::RaftHeartbeatCommand(RaftState& s) : state_(s)
{

}

// Heartbets can be received in two states: Candidate and Follower
// When in candidate state is noe recived heartbeat message it means that new leader was elected and it must
// transition to Follower state.
// If HB received in Follower state we need to re-arm heartbeat timer.
boost::property_tree::ptree RaftHeartbeatCommand::operator()()
{
    boost::property_tree::ptree result;

    std::cout << "♥" << std::endl;

    if (state_.get_type() == RaftStateType::Candidate)
        {
        state_.set_next_state_follower();
        auto cs = dynamic_cast<RaftCandidateState*>(&state_);
        if (cs != nullptr)
            cs->cancel_election(); // Election timer could be running, stop it before deleting state.
        }


    if (state_.get_type() == RaftStateType::Follower)
        {
        auto fs = dynamic_cast<RaftFollowerState*>(&state_);
        if (fs != nullptr)
            fs->rearm_heartbeat_timer();
        }

    return result;
}
