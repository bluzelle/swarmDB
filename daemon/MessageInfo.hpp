#ifndef BLUZELLE_MESSAGEINFO_H
#define BLUZELLE_MESSAGEINFO_H

#include <string>

using std::string;

class MessageInfo {
public:
    string destination_;
    string message_;
    boost::posix_time::posix_time_system timestamp_;
};

#endif //BLUZELLE_MESSAGEINFO_H
