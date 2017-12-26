#ifndef BLUZELLE_RAFTHEARTBEATCOMMAND_H
#define BLUZELLE_RAFTHEARTBEATCOMMAND_H

#include "Command.hpp"
#include "RaftState.h"

class RaftHeartbeatCommand : public Command
{
private:
    RaftState& state_;

public:
    RaftHeartbeatCommand(RaftState& s);
    virtual boost::property_tree::ptree operator()();
};

#endif //BLUZELLE_RAFTHEARTBEATCOMMAND_H
