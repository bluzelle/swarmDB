#ifndef BLUZELLE_COMMANDPROCESSOR_H
#define BLUZELLE_COMMANDPROCESSOR_H

#include "NodeInfo.hpp"
#include "Storage.h"
#include "Command.hpp"
#include "ApiCommandQueue.h"

#include <memory>

using std::unique_ptr;

enum State {
    undetermined,
    leader,
    follower
};

class CommandFactory {
private:
    Storage& storage_;
    ApiCommandQueue& queue_;

    bool is_raft(const boost::property_tree::ptree& s) const;
    bool is_crud(const boost::property_tree::ptree& s) const;
    bool is_api(const boost::property_tree::ptree& s) const;

    unique_ptr<Command> make_raft_command(const boost::property_tree::ptree& s) const;
    unique_ptr<Command> make_crud_command(const boost::property_tree::ptree& s) const;
    unique_ptr<Command> make_api_command(const boost::property_tree::ptree& s) const;

    std::pair<string,string> get_data(const boost::property_tree::ptree& pt) const;

public:
    State
    state() const;

    CommandFactory(
            Storage& st,
            ApiCommandQueue& queue
    );

    unique_ptr<Command>
    get_command(
            const boost::property_tree::ptree& pt
    ) const;
};

#endif //BLUZELLE_COMMANDPROCESSOR_H
