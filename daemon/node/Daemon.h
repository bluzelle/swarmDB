#ifndef BLUZELLE_DAEMON_H
#define BLUZELLE_DAEMON_H

#include "DaemonInfo.h"

#include <string>
#include <sstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

// TODO: This is unused right now, but will be used in main.

class Daemon
{
    DaemonInfo *daemon_info_;
public:
    Daemon()
    {
        daemon_info_ = &DaemonInfo::get_instance();
        boost::uuids::basic_random_generator<boost::mt19937> gen;
        const boost::uuids::uuid node_id{gen()};
        daemon_info_->id() = node_id;
    }

    ~Daemon() = default;

    std::string node_id()
    {
        return boost::lexical_cast<std::string>(DaemonInfo::get_instance().id());
    }

    std::string ethereum_address()
    {
        return DaemonInfo::get_instance().ethereum_address();
    }

    uint16_t get_port()
    {
        return DaemonInfo::get_instance().host_port();
    }

    uint16_t get_token_balance()
    {
        return 0;
    }

    std::string get_info()
    {
        std::stringstream ss;
        ss << "Running node with ID: "
           << node_id() << "\n"
           << " Ethereum Address ID: "
           << ethereum_address() << "\n"
           << "             on port: " << get_port() << "\n"
           << "       Token Balance: " << get_token_balance() << " BLZ\n";
        return ss.str();
    }
};

#endif //BLUZELLE_DAEMON_H
