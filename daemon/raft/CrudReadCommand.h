#ifndef BLUZELLE_CRUDREADCOMMAND_H
#define BLUZELLE_CRUDREADCOMMAND_H

#include "Command.hpp"

class CrudReadCommand : public Command {
public:
    Storage& storage_;
    string key_;

public:
    CrudReadCommand(Storage& s, string k);
    virtual boost::property_tree::ptree operator()();
};

#endif //BLUZELLE_CRUDREADCOMMAND_H
