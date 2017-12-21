#include <iostream>

#include "RaftHeartbeatCommand.h"

boost::property_tree::ptree RaftHeartbeatCommand::operator()() {
    boost::property_tree::ptree result;

    std::cout << " ♥" << std::endl;

    return result;
}
