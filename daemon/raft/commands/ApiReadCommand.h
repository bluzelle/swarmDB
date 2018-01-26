#ifndef BLUZELLE_APIREADCOMMAND_H_H
#define BLUZELLE_APIREADCOMMAND_H_H

#include "Command.hpp"
#include "ApiCommandQueue.h"
#include "Storage.h"

class ApiReadCommand : public Command
{

private:
    ApiCommandQueue& queue_;
    Storage& storage_;
    boost::property_tree::ptree pt_;

public:

    ApiReadCommand
        (
            ApiCommandQueue& q,
            Storage& s,
            boost::property_tree::ptree pt
        );

    virtual
    boost::property_tree::ptree
    operator()();
};

#endif //BLUZELLE_APIREADCOMMAND_H_H
