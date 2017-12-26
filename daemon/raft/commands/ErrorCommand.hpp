#ifndef BLUZELLE_ERRORCOMMAND_H
#define BLUZELLE_ERRORCOMMAND_H

#include "Command.hpp"

class ErrorCommand : public Command {
private:
    string message_;

public:
    ErrorCommand(const string& err) : message_(err) {}
    virtual boost::property_tree::ptree operator()() {
        return error(message_);
    }
};


#endif //BLUZELLE_ERRORCOMMAND_H
