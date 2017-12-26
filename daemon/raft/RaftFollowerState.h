#ifndef BLUZELLE_RAFTFOLLOWERSTATE_H
#define BLUZELLE_RAFTFOLLOWERSTATE_H

#include <boost/asio/deadline_timer.hpp>

#include "RaftState.h"

class RaftFollowerState : public RaftState {
private:
    boost::asio::deadline_timer heartbeat_timer_; // When expired change state to Candidate.

    void heartbeat_timer_expired(const boost::system::error_code& e);

public:
    RaftFollowerState(boost::asio::io_service& ios,
                       Storage& s,
                       CommandFactory& cf,
                       ApiCommandQueue& pq,
                       PeerList& ps,
                       function<string(const string&)> rh,
                       unique_ptr<RaftState>& ns);

    ~RaftFollowerState();

    virtual unique_ptr<RaftState> handle_request(const string& request, string& response) override;

    virtual RaftStateType get_type() const override { return RaftStateType::Follower; }

    void rearm_heartbeat_timer();
};

#endif //BLUZELLE_RAFTFOLLOWERSTATE_H
