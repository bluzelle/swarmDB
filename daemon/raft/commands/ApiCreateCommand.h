#ifndef BLUZELLE_APICREATECOMMAND_H
#define BLUZELLE_APICREATECOMMAND_H

#include "Command.hpp"
#include "ApiCommandQueue.h"
#include "Storage.h"

class ApiCreateCommand : public Command
{
    ApiCommandQueue& queue_;
    Storage& storage_;
    boost::property_tree::ptree pt_;

public:
    ApiCreateCommand
        (
            ApiCommandQueue& q,
            Storage& s,
            boost::property_tree::ptree pt
        );

    virtual boost::property_tree::ptree operator()();
};

#endif //BLUZELLE_APICREATECOMMAND_H
