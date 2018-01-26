#ifndef BLUZELLE_NODEINFO_H
#define BLUZELLE_NODEINFO_H

#include <string>
#include <boost/lexical_cast.hpp>

using namespace std;

class NodeInfo {
    string name_;
    string host_;
    uint16_t    port_;

public:
    NodeInfo(
        NodeInfo const &
    ) = default;

    NodeInfo() : name_{""}, host_{""}, port_{0}
    {
    }


    string&
    name()
    {
        return name_;
    }

    string&
    host()
    {
        return host_;
    }

    uint16_t&
    port()
    {
        return port_;
    }
};

#endif //BLUZELLE_NODEINFO_H
