#ifndef BLUZELLE_NODEINFO_H
#define BLUZELLE_NODEINFO_H

#include <string>
#include<vector>

#include "boost/date_time/posix_time/posix_time_types.hpp"

using std::string;
using std::vector;

enum class State {
    unknown,
    candidate,
    leader,
    follower,
};

class NodeInfo {
public:
    string host_;
    ushort port_;

    string address_;
    State state_;

    string name_;
    string config_;

    boost::posix_time::posix_time_system last_contacted_;

    NodeInfo(const string& host,    // IP address or name.
             ushort port,           // Port.
             const string& address, // Etherium address
             const string& name)    // Node name (description).
    : host_(host), port_(port), address_(address), state_(State::unknown), name_(name) {}

    NodeInfo() {
        host_ = "localhost";
        port_ = 58000;
        address_ = "0x0000000000000000000000000000000000000000";
        name_ = "this computer";
    }


    static NodeInfo this_node() { return NodeInfo();}
};

#endif //BLUZELLE_NODEINFO_H
