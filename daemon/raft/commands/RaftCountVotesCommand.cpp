#include "RaftCountVotesCommand.h"

RaftCountVotesCommand::RaftCountVotesCommand(RaftState& s, bool yes)
        : state_(s), voted_yes_(yes)
{

}

boost::property_tree::ptree RaftCountVotesCommand::operator()()
{
    boost::property_tree::ptree result;

    RaftCandidateState* s = dynamic_cast<RaftCandidateState*>(&state_);
    if (s != nullptr)
        {
        s->count_vote(voted_yes_);
        }

    return result;
}