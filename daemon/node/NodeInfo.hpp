#ifndef BLUZELLE_NODEINFO_H
#define BLUZELLE_NODEINFO_H

#include <string>
#include <map>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <raft/CommandFactory.h>

using std::string;
using std::vector;
using std::map;
using boost::lexical_cast;

class NodeInfo {
    map<string, string> values_;
public:
    NodeInfo(NodeInfo const &) = default;
    NodeInfo() = default;

    template <typename T>
    const T
    get_value(const string &key)
    {
        return lexical_cast<T>(values_[key]);
    }

    template <typename T>
    void set_value(const string &key, const T &value)
    {
        values_[key] = boost::lexical_cast<std::string>(value);
    }
};

#endif //BLUZELLE_NODEINFO_H
