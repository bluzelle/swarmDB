#ifndef BLUZELLE_COMMAND_H
#define BLUZELLE_COMMAND_H

#include "Storage.h"
#include <boost/property_tree/ptree.hpp>
#include <string>

using std::string;

class Command {
public:
    virtual boost::property_tree::ptree operator()() = 0;

    boost::property_tree::ptree success() {
        return result("success");
    }

    boost::property_tree::ptree result(string res) {
        boost::property_tree::ptree pt;
        pt.put("result", res);
        return pt;
    }

    boost::property_tree::ptree error(string er) {
        boost::property_tree::ptree pt;
        pt.put("error", er);
        return pt;
    }
};

#endif //BLUZELLE_COMMAND_H
