#include "CommandFactory.h"
#include "RaftHeartbeatCommand.h"
#include "CrudCreateCommand.h"
#include "CrudReadCommand.h"
#include "ApiCreateCommand.h"
#include "ApiReadCommand.h"
//#include "ErrorCommand.hpp"
#include "RaftVoteCommand.h"
#include "RaftCountVotesCommand.h"

using namespace std;

CommandFactory::CommandFactory
    (
        Storage& st,
        ApiCommandQueue& q
    )
    : storage_(st), queue_(q)
{

}

pair<string,string>
CommandFactory::get_data
    (
        const boost::property_tree::ptree& pt
    ) const
{
    auto data  = pt.get_child("data.");

    string key = ( data.count("key") > 0 ? data.get<string>("key") : "" );

    string val = ( data.count("value") > 0 ? data.get<string>("value") : "");

    return make_pair<string,string>(std::move(key), std::move(val));
}

unique_ptr<Command>
CommandFactory::get_command(
    const boost::property_tree::ptree& pt,
    RaftState& st) const
{
    if (has_key(pt, "bzn-api"))
        {
        return make_api_command(pt, st);
        }

    if (has_key(pt, "crud"))
        {
        return make_crud_command(pt, st);
        }

    if (has_key(pt, "raft"))
        {
        return make_raft_command(pt, st);
        }

    if (has_key(pt, "cmd"))
        {
        return make_command(pt, st);
        }
    return nullptr;
}

bool
CommandFactory::has_key(
    const boost::property_tree::ptree& pt, const string& k
) const
{
    return pt.find(k) != pt.not_found();
}

unique_ptr<Command>
CommandFactory::make_raft_command(
    const boost::property_tree::ptree& pt,
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
CommandFactory::make_crud_command(
    const boost::property_tree::ptree& pt,
    RaftState& st) const
{
    std::cerr << "RaftState " << &st << " is unused in make_crud_command.\n";
    auto cmd = pt.get<string>("crud");
    auto dat = get_data(pt);

    if (cmd == "create")
        return std::make_unique<CrudCreateCommand>(storage_, dat.first, dat.second);

    if (cmd == "read")
        return std::make_unique<CrudReadCommand>(storage_, dat.first);

    return nullptr;
}

unique_ptr<Command>
CommandFactory::make_api_command(
    const boost::property_tree::ptree& pt,
    RaftState& st) const
{
    std::cerr << "RaftState " << &st << " is unused in make_api_command.\n";
    auto cmd = pt.get<string>("bzn-api");
    auto dat = get_data(pt);

    if (cmd == "create")
        return std::make_unique<ApiCreateCommand>(queue_, storage_, pt);

    if (cmd == "read")
        return std::make_unique<ApiReadCommand>(queue_, storage_, pt);

    return nullptr;
}

unique_ptr<Command>
CommandFactory::make_command(
    const boost::property_tree::ptree& pt,
    RaftState& st
) const
{
    auto cmd = pt.get<string>("cmd");
    static std::map<std::string, std::function<void()>> commands;

    cerr << "st [" << &st << "] should be used\n";

    if(commands.size()==0)
        {
        commands["ping"] = [](){
            std::cout << "ping\n";
            return 1;
        };
        commands["request-vote"] = []()
        {
            std::cout << "request-vote\n";
            return 2;
        };
        commands["vote"] = [](){
            std::cout << "request-vote\n";
            return 3;
        };
        }

    return nullptr;
}