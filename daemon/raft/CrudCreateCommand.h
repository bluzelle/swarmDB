#ifndef BLUZELLE_CRUDCOMMAND_H
#define BLUZELLE_CRUDCOMMAND_H

#include <tuple>
#include <string>

using std::tuple;
using std::string;

#include "Command.hpp"
#include "Storage.h"

class CrudCreateCommand : public Command {
public:
    Storage& storage_;
    string key_;
    string value_;

public:
    CrudCreateCommand(Storage& s, string k, string v);
    virtual boost::property_tree::ptree operator()();
};

#endif //BLUZELLE_CRUDCOMMAND_H
