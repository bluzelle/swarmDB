#include "DaemonInfo.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;

namespace pt = boost::property_tree;

boost::uuids::uuid&
DaemonInfo::id()
{
    return id_;
}

string&
DaemonInfo::host_name()
{
    return node_info_.name();
}

string&
DaemonInfo::host_ip()
{
    return node_info_.host();
}

uint16_t&
DaemonInfo::host_port()
{
    return node_info_.port();
}

string&
DaemonInfo::ethereum_address()
{
    return ethereum_address_;
};

uint64_t&
DaemonInfo::ropsten_token_balance()
{
    return ropsten_token_balance_;
}
