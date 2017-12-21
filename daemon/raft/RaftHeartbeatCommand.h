#ifndef BLUZELLE_RAFTHEARTBEATCOMMAND_H
#define BLUZELLE_RAFTHEARTBEATCOMMAND_H

#include "Command.hpp"

class RaftHeartbeatCommand : public Command {
public:
    virtual boost::property_tree::ptree operator()();
};


#endif //BLUZELLE_RAFTHEARTBEATCOMMAND_H
