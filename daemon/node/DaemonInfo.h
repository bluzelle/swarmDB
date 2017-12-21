#ifndef BLUZELLE_DAEMONINFO_H
#define BLUZELLE_DAEMONINFO_H

#include "node/NodeInfo.hpp"
#include "node/Singleton.h"

class DaemonInfo final : public Singleton<DaemonInfo>
{
    NodeInfo node_info_;
    friend class Singleton<DaemonInfo>;
public:
    template <typename T>
    const T
    get_value(const string &key)
    {
        return node_info_.get_value<T>(key);
    }

    template <typename T>
    void set_value(const string &key, const T &value)
    {
        node_info_.set_value(key, value);
    }
};

#endif //BLUZELLE_DAEMONINFO_H
