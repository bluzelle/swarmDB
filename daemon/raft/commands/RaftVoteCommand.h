#ifndef BLUZELLE_RAFTVOTECOMMAND_H
#define BLUZELLE_RAFTVOTECOMMAND_H

#include "Command.hpp"
#include "RaftCandidateState.h"

class RaftVoteCommand : public Command
{
private:
    RaftState& state_;

public:
    RaftVoteCommand(RaftState& s);
    virtual boost::property_tree::ptree operator()();
};

#endif //BLUZELLE_RAFTVOTECOMMAND_H
