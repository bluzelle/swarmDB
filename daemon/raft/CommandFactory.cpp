#include "CommandFactory.h"

#include "RaftHeartbeatCommand.h"
#include "CrudCreateCommand.h"
#include "CrudReadCommand.h"
#include "ApiCreateCommand.h"
#include "ApiReadCommand.h"
#include "ErrorCommand.hpp"
#include "DaemonInfo.h"


CommandFactory::CommandFactory(Storage& st, ApiCommandQueue& q)
    : storage_(st), queue_(q) {

}

unique_ptr<Command> CommandFactory::get_command(const boost::property_tree::ptree& pt) const {
    if (is_raft(pt))
        return make_raft_command(pt);

    if (is_crud(pt))
        return make_crud_command(pt);

    if (is_api(pt))
        return make_api_command(pt);

    return std::make_unique<ErrorCommand>("Unsupported command");
}

// {"raft":"beep"} // This is a heartbeat command.
bool CommandFactory::is_raft(const boost::property_tree::ptree& pt) const {
    return pt.find("raft") != pt.not_found();
}

// CRUD commands go from leader to followers. I.e they are log replication commands.
// {"crud":"create", "transaction-id":"123", "data":{"key":"key_one", "value":"value_one"}}
bool CommandFactory::is_crud(const boost::property_tree::ptree& pt) const {
    return pt.find("crud") != pt.not_found();
}

// API commands go from API to leader. Same format as CRUD.
// In future we can change either API or CRUD command format so better keep them separate.
// {"bzn-api":"create", "transaction-id":"123", "data":{"key":"key_one", "value":"value_one"}}
bool CommandFactory::is_api(const boost::property_tree::ptree& pt) const {
    return pt.find("bzn-api") != pt.not_found();
}

unique_ptr<Command> CommandFactory::make_raft_command(const boost::property_tree::ptree& pt) const {
    auto cmd = pt.get<string>("raft");
    if (cmd == "beep")
        {
        if (state() == State::follower)
            return std::make_unique<RaftHeartbeatCommand>();
        }

    return std::make_unique<ErrorCommand>("Leader received heartbeat");
}

unique_ptr<Command> CommandFactory::make_crud_command(const boost::property_tree::ptree& pt) const {
    if (state() == State::leader)
        return std::make_unique<ErrorCommand>("CRUD request cannot be sent to swarm leader");

    auto cmd = pt.get<string>("crud");
    auto dat = get_data(pt);

    if (cmd == "create")
        return std::make_unique<CrudCreateCommand>(storage_, dat.first, dat.second);

    if (cmd == "read")
        return std::make_unique<CrudReadCommand>(storage_, dat.first);

    return nullptr;
}

unique_ptr<Command> CommandFactory::make_api_command(const boost::property_tree::ptree& pt) const {
    //if (state_ == State::follower)
    //    return std::make_unique<ErrorCommand>("API request must be sent to swarm leader");

    auto cmd = pt.get<string>("bzn-api");
    auto dat = get_data(pt);

    if (cmd == "create")
        return std::make_unique<ApiCreateCommand>(queue_, storage_, pt);

    if (cmd == "read")
        return std::make_unique<ApiReadCommand>(queue_, storage_, pt);

    return nullptr;
}


std::pair<string,string> CommandFactory::get_data(const boost::property_tree::ptree& pt) const {
    auto data  = pt.get_child("data.");

    string key;
    if (data.count("key") > 0)
        key = data.get<string>("key");

    string val;
    if (data.count("value") > 0)
        val = data.get<string>("value");

    return std::make_pair<string,string>(std::move(key), std::move(val));
}

State CommandFactory::state() const {
    DaemonInfo& daemon_info = DaemonInfo::get_instance();
    return (State)daemon_info.get_value<int>("state");
}