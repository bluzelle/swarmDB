#ifndef BLUZELLE_APIUPDATENODESCOMMAND_H
#define BLUZELLE_APIUPDATENODESCOMMAND_H

#include "Command.hpp"
#include "ApiCommandQueue.h"
#include "Storage.h"

class ApiUpdateNodesCommand : Command
{
    ApiCommandQueue&            queue_;
    Storage&                    storage_;
    boost::property_tree::ptree pt_;

public:
    ApiUpdateNodesCommand
        (
            ApiCommandQueue& q,
            Storage& s,
            boost::property_tree::ptree pt
        ) :
        queue_(q),
        storage_(s),
        pt_(pt)
    {
        cerr << "queue_["<< &queue_ <<"] should be used.\n";
        cerr << "storage_["<< &storage_ <<"] should be used.\n";
    }

    virtual boost::property_tree::ptree
    operator()()
    {
        return pt_;
    }

};


#endif //BLUZELLE_APIUPDATENODESCOMMAND_H
