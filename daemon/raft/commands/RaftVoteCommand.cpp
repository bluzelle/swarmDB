#include "RaftVoteCommand.h"
#include "JsonTools.h"

static constexpr char s_vote_yes_message[] = "{\"raft\":\"vote\", \"data\":{\"voted\":\"yes\"}}";
static constexpr char s_vote_no_message[] = "{\"raft\":\"vote\", \"data\":{\"voted\":\"no\"}}";

RaftVoteCommand::RaftVoteCommand(RaftState& s) : state_(s)
{

}

#include <chrono>
#include <thread>

boost::property_tree::ptree RaftVoteCommand::operator()()
{
    boost::property_tree::ptree result;

    RaftCandidateState* s = dynamic_cast<RaftCandidateState*>(&state_);
    if (s != nullptr)
        {
        if (s->nominated_self())
            {
            std::cout << "voted 'no'" << std::endl;
            result = pt_from_json_string(s_vote_no_message);
            }
        else
            {
            std::cout << "voted 'yes'" << std::endl;
            result = pt_from_json_string(s_vote_yes_message);
            }
        s->cancel_election(); // No need to nominate self if there is already a nomination.
        }

    return result;
}