#ifndef BLUZELLE_RAFTCANDIDATESTATE_H
#define BLUZELLE_RAFTCANDIDATESTATE_H

#include <random>

#include <boost/asio/deadline_timer.hpp>

#include "RaftState.h"

class RaftCandidateState : public RaftState {
private:
    std::random_device rd_;
    boost::asio::deadline_timer election_timeout_timer_;

    void schedule_election();
    void start_election(const boost::system::error_code& e);
    void finish_election();

    bool nominated_for_leader_;

    uint voted_yes_;
    uint voted_no_;

public:
    RaftCandidateState(boost::asio::io_service& ios,
                       Storage& s,
                       CommandFactory& cf,
                       ApiCommandQueue& pq,
                       PeerList& ps,
                       function<string(const string&)> rh,
                       unique_ptr<RaftState>& ns);

    ~RaftCandidateState();

    bool nominated_self() {return nominated_for_leader_; }
    void count_vote(bool vote_yes);

    virtual unique_ptr<RaftState> handle_request(const string& request, string& response) override;

    virtual RaftStateType get_type() const override { return RaftStateType::Candidate; }

    void cancel_election();
};

#endif //BLUZELLE_RAFTCANDIDATESTATE_H
