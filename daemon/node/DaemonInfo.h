#ifndef BLUZELLE_DAEMONINFO_H
#define BLUZELLE_DAEMONINFO_H

#include "node/NodeInfo.hpp"
#include "node/Singleton.h"

#include <vector>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/uuid/uuid.hpp>

namespace pt = boost::property_tree;

class
DaemonInfo final : public Singleton<DaemonInfo>
{
    NodeInfo            node_info_;

    std::string         ethereum_address_;

    boost::uuids::uuid  id_;

    uint64_t            ropsten_token_balance_;

    friend class Singleton<DaemonInfo>;

    DaemonInfo() = default;

public:

    boost::uuids::uuid&
    id()
    {
        return id_;
    }

    std::string&
    host_name()
    {
        return node_info_.name();
    }

    std::string&
    host_ip()
    {
        return node_info_.host();
    }

    uint16_t&
    host_port()
    {
        return node_info_.port();
    }

    std::string&
    ethereum_address()
    {
        return ethereum_address_;
    };

    uint64_t&
    ropsten_token_balance()
    {
        return ropsten_token_balance_;
    }

};

#endif //BLUZELLE_DAEMONINFO_H
