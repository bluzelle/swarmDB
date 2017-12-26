#include <iostream>

#include <boost/asio/placeholders.hpp>

#include "ApiCommandQueue.h"
#include "CommandFactory.h"
#include "PeerList.h"
#include "RaftLeaderState.h"
#include "JsonTools.h"

static constexpr char s_heartbeat_message[] = "{\"raft\": \"beep\"}";

RaftLeaderState::RaftLeaderState(boost::asio::io_service& ios,
                                 Storage& s,
                                 CommandFactory& cf,
                                 ApiCommandQueue& pq,
                                 PeerList& ps,
                                 function<string(const string&)> rh,
                                 unique_ptr<RaftState>& ns)
        : RaftState(ios, s, cf, pq, ps, rh, ns),
          heartbeat_timer_(ios_,
                           boost::posix_time::milliseconds(raft_default_heartbeat_interval_milliseconds))
{
    std::cout << "          I am Leader" << std::endl;
    heartbeat_timer_.async_wait(boost::bind(&RaftLeaderState::heartbeat,
                                            this, boost::asio::placeholders::error));
}

RaftLeaderState::~RaftLeaderState()
{
    heartbeat_timer_.cancel();
}

void RaftLeaderState::heartbeat(const boost::system::error_code& e)
{
    if (e == boost::asio::error::operation_aborted)
        return;

    if (peer_queue_.empty())
        {
        std::cout << "♥ ";
        for (auto &p : peers_)
            {
            p.send_request(s_heartbeat_message, handler_, false);
            std::cout << ".";
            }
        }
    else
        {
        std::cout << "❥ ";
        auto m = peer_queue_.front();
        for (auto &p : peers_)
            {
            p.send_request(m.second, handler_, false);
            std::cout << ".";
            }
        peer_queue_.pop();
        }

    std::cout << std::endl;

    // Re-arm timer.
    heartbeat_timer_.cancel();
    heartbeat_timer_.expires_from_now(
            boost::posix_time::milliseconds(raft_default_heartbeat_interval_milliseconds));

    heartbeat_timer_.async_wait(
            boost::bind(&RaftLeaderState::heartbeat,
                        this, boost::asio::placeholders::error));
}


unique_ptr<RaftState> RaftLeaderState::handle_request(const string& request, string& response)
{
    auto pt = pt_from_json_string(request);

    unique_ptr<Command> command = command_factory_.get_command(pt, *this);
    if (command != nullptr)
        response = pt_to_json_string(command->operator()());

    return nullptr;
}
