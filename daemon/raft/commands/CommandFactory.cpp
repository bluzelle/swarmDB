#include "CommandFactory.h"

#include "RaftHeartbeatCommand.h"
#include "CrudCreateCommand.h"
#include "CrudReadCommand.h"
#include "ApiCreateCommand.h"
#include "ApiReadCommand.h"
#include "ErrorCommand.hpp"
#include "RaftVoteCommand.h"
#include "RaftCountVotesCommand.h"


CommandFactory::CommandFactory(Storage& st, ApiCommandQueue& q)
    : storage_(st), queue_(q) {

}

std::pair<string,string>
CommandFactory::get_data(const boost::property_tree::ptree& pt) const {
    auto data  = pt.get_child("data.");

    string key;
    if (data.count("key") > 0)
        key = data.get<string>("key");

    string val;
    if (data.count("value") > 0)
        val = data.get<string>("value");

    return std::make_pair<string,string>(std::move(key), std::move(val));
}

unique_ptr<Command>
CommandFactory::get_command(const boost::property_tree::ptree& pt,
                      RaftState& st) const
{
    if (has_key(pt, "bzn-api"))
        return make_api_command(pt, st);

    if (has_key(pt, "crud"))
        return make_crud_command(pt, st);

    if (has_key(pt, "raft"))
        return make_raft_command(pt, st);
}

bool CommandFactory::has_key(const boost::property_tree::ptree& pt, const string& k) const
{
    return pt.find(k) != pt.not_found();
}

unique_ptr<Command>
CommandFactory::make_raft_command(const boost::property_tree::ptree& pt,
                                  RaftState& st) const
{
    auto cmd = pt.get<string>("raft");

    // Candidate receive request for vote.
    if (cmd == "request-vote")
        return std::make_unique<RaftVoteCommand>(st);

    // Candidate received YES/NO vote.
    if (cmd == "vote")
        {
        auto data  = pt.get_child("data.");
        bool voted_yes = data.get<string>("voted") == "yes" ? true : false;
        return std::make_unique<RaftCountVotesCommand>(st, voted_yes);
        }

    // Candidate received heartbeat (new leader was elected).
    if (cmd == "beep")
        return std::make_unique<RaftHeartbeatCommand>(st);

    return nullptr;
}

unique_ptr<Command>
CommandFactory::make_crud_command(const boost::property_tree::ptree& pt,
                                  RaftState& st) const {
    auto cmd = pt.get<string>("crud");
    auto dat = get_data(pt);

    if (cmd == "create")
        return std::make_unique<CrudCreateCommand>(storage_, dat.first, dat.second);

    if (cmd == "read")
        return std::make_unique<CrudReadCommand>(storage_, dat.first);

    return nullptr;
}

unique_ptr<Command>
CommandFactory::make_api_command(const boost::property_tree::ptree& pt,
                                 RaftState& st) const {
    auto cmd = pt.get<string>("bzn-api");
    auto dat = get_data(pt);

    if (cmd == "create")
        return std::make_unique<ApiCreateCommand>(queue_, storage_, pt);

    if (cmd == "read")
        return std::make_unique<ApiReadCommand>(queue_, storage_, pt);

    return nullptr;
}

