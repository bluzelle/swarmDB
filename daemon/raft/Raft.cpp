#include "Raft.h"
#include "JsonTools.h"
#include "DaemonInfo.h"

#include <iostream>
#include <utility>
#include <thread>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>

static boost::uuids::uuid s_transaction_id;

DaemonInfo& daemon_info = DaemonInfo::get_instance();

Raft::Raft(boost::asio::io_service &ios)
        : ios_(ios),
          peers_(ios_),
          storage_("./storage_" + daemon_info.get_value<std::string>("name") + ".txt"), // TODO: using the wrong info..
          command_factory_(
                  storage_,
                  peer_queue_),
          heartbeat_timer_(ios_,
                           boost::posix_time::milliseconds(raft_default_heartbeat_interval_milliseconds))
{
    daemon_info.set_value("state", State::undetermined);
    static boost::uuids::nil_generator nil_uuid_gen;
    s_transaction_id = nil_uuid_gen();
}


void Raft::start_heartbeat()
{
    heartbeat_timer_.async_wait
            (
                    boost::bind
                            (
                                    &Raft::heartbeat,
                                    this
                            )
            );
}

void Raft::announce_follower()
{
    std::cout << "\n\tI am follower" << std::endl;
}



void Raft::run() {
    // Leader is hardcoded: node with port ending with '0' is always a leader.
    daemon_info.set_value<int>("state", (daemon_info.get_value<int>("port") % 10 == 0 ? State::leader : State::follower));
    (State)daemon_info.get_value<int>("state") == State::leader ? start_heartbeat() : announce_follower();

    // How CRUD works? Am I writing to any node and it sends it to leader or I can write to leader only.
    // All goes through leader. Leader receives log entry and writes it locally, its state is uncommited.
    // Then leader sends the entry to all followers and waits for confirmation. When confirmation received
    // from majority the state changed to 'committed'. Then leader notify followers that entry is committed.
}

//void Raft::start_leader_election() {
/*
 * If node haven't heard from leader it can start election.
 * Change state to State::candidate and request votes
*/
    // Election_Timeout -> time to wait before starting new election (become a candidate)
    // Random in 150-300 ms interval

    // After Election_Timeout follower becomes candidate and start election term, votes for itself and sends
    // Request_Vote messages to other nodes.
    // If node hasn't voted for itself or didn't reply to others node Request_Vote it votes "YES" otherwise "NO"
    // An resets election timeout (won't start new election).
    // When candidate received majority of votes it sets itself as leader.
    // The leader sends Append_Entry messages to followers in Heartbeat_Timeout intervals. Followers respond
    // If follower don't receive Append_Entry in time alotted new election term starts.
    // Handle Split_Vote
//}

void Raft::heartbeat() {
    if (peer_queue_.empty())
        {
        std::cout << "♥ ";
        for (auto &p : peers_)
            {
            p.send_request(s_heartbeat_message);
            std::cout << ".";
            }
        }
    else
        {
        std::cout << "❥ ";
        auto m = peer_queue_.front();
        for (auto &p : peers_)
            {
            p.send_request(m.second);
            std::cout << ".";
            }
        peer_queue_.pop();
        }
    std::cout << std::endl;

    // Re-arm timer.
    heartbeat_timer_.expires_at(
            heartbeat_timer_.expires_at() +
            boost::posix_time::milliseconds(raft_default_heartbeat_interval_milliseconds));
    heartbeat_timer_.async_wait(
            boost::bind(&Raft::heartbeat,
                        this));
}

string Raft::handle_request(const string &req) {
    auto pt = pt_from_json_string(req);

    unique_ptr<Command> command = command_factory_.get_command(pt);
    string response = pt_to_json_string(command->operator()());

    return response;
}



