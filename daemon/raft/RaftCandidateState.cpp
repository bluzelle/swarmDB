#include <iostream>

#include <boost/asio/placeholders.hpp>

#include "PeerList.h"
#include "CommandFactory.h"
#include "RaftCandidateState.h"
#include "RaftLeaderState.h"
#include "JsonTools.h"

static constexpr char s_request_for_vote_message[] = "{\"raft\": \"request-vote\"}";


RaftCandidateState::RaftCandidateState(boost::asio::io_service& ios,
                                       Storage& s,
                                       CommandFactory& cf,
                                       ApiCommandQueue& pq,
                                       PeerList& ps,
                                       function<string(const string&)> rh,
                                       unique_ptr<RaftState>& ns)
        : RaftState(ios, s, cf, pq, ps, rh, ns),
          election_timeout_timer_(ios_, boost::posix_time::milliseconds(
                  raft_election_timeout_interval_min_milliseconds)),
          nominated_for_leader_(false),
          voted_yes_(0),
          voted_no_(0)
{
    std::cout << "          I am Candidate" << std::endl;

    schedule_election();
}

RaftCandidateState::~RaftCandidateState()
{
    cancel_election();
}

void RaftCandidateState::schedule_election()
{
    std::mt19937 rng(rd_());
    std::uniform_int_distribution<uint> uni(1000,
                                            raft_election_timeout_interval_max_milliseconds -
                                                    raft_election_timeout_interval_min_milliseconds);

    uint election_in = uni(rng);

    std::cout << "Election in " << boost::lexical_cast<string>(election_in) << " millisecs" << std::endl;

    election_timeout_timer_.cancel();
    election_timeout_timer_.expires_from_now(boost::posix_time::milliseconds(
            election_in));

    election_timeout_timer_.async_wait(boost::bind(&RaftCandidateState::start_election,
                                                   this, boost::asio::placeholders::error));
}

void RaftCandidateState::cancel_election()
{
    election_timeout_timer_.cancel();
}

void RaftCandidateState::start_election(const boost::system::error_code& e)
{
    if (e == boost::asio::error::operation_aborted)
        {
        std::cout << "election cancelled" << std::endl;
        return; // Timer was cancelled.
        }


    nominated_for_leader_ = true;
    std::cout << "votes requested ";
    for (auto &p : peers_)
        {
        p.send_request(s_request_for_vote_message, handler_, true);
        std::cout << ".";
        }

    std::cout << std::endl;
}

void RaftCandidateState::finish_election()
{
    nominated_for_leader_ = false;
    voted_yes_ = 0;
    voted_no_ = 0;
}

void RaftCandidateState::count_vote(bool vote_yes)
{
    if (vote_yes)
        {
        std::cout << "  'yes' received" << std::endl;
        ++ voted_yes_;
        }
    else
        {
        std::cout << "  'no' received" << std::endl;
        ++voted_no_;
        }

    if (voted_yes_ >= peers_.size() * 2 / 3) // If 2/3rd voted yes this node is the new leader.
        {
        std::cout << "Election finished" << std::endl;
        finish_election();
        next_state_ = std::make_unique<RaftLeaderState>(ios_,
                                                        storage_,
                                                        command_factory_,
                                                        peer_queue_,
                                                        peers_,
                                                        handler_,
                                                        next_state_);
        }
}

// If heartbeat received transition to follower state.
// If request for vote received send YES/NO vote. YES if this node didn't nominate itself to be a leader.
// If vote received count votes and if majority reached transition to leader state if not re-start election.
unique_ptr<RaftState> RaftCandidateState::handle_request(const string& request, string& response)
{
    auto pt = pt_from_json_string(request);

    unique_ptr<Command> command = command_factory_.get_command(pt, *this);
    if (command != nullptr)
        response = pt_to_json_string(command->operator()());

    return nullptr;
}
