#ifndef BLUZELLE_RAFTLEADERSTATE_H
#define BLUZELLE_RAFTLEADERSTATE_H

#include <boost/asio/deadline_timer.hpp>

#include "RaftState.h"

class RaftLeaderState : public RaftState
{
private:
    boost::asio::deadline_timer heartbeat_timer_;
    void heartbeat(const boost::system::error_code& e);

public:
    RaftLeaderState(boost::asio::io_service& ios,
                    Storage& s,
                    CommandFactory& cf,
                    ApiCommandQueue& pq,
                    PeerList& ps,
                    function<string(const string&)> rh,
                    unique_ptr<RaftState>& ns);

    ~RaftLeaderState();

    virtual unique_ptr<RaftState> handle_request(const string& request, string& response) override;

    virtual RaftStateType get_type() const override { return RaftStateType::Leader; }
};

#endif //BLUZELLE_RAFTLEADERSTATE_H
