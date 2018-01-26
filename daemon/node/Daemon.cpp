#include "Daemon.h"

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace std;

Daemon::Daemon()
{
    daemon_info_ = &DaemonInfo::get_instance();
    boost::uuids::basic_random_generator<boost::mt19937> gen;
    const boost::uuids::uuid node_id{gen()};
    daemon_info_->id() = node_id;
}

string Daemon::node_id()
{
    return boost::lexical_cast<string>(DaemonInfo::get_instance().id());
}

string Daemon::ethereum_address()
{
    return DaemonInfo::get_instance().ethereum_address();
}

uint16_t Daemon::get_port()
{
    return DaemonInfo::get_instance().host_port();
}

uint16_t Daemon::get_token_balance()
{
    return 0;
}

string Daemon::get_info()
{
    stringstream ss;
    ss << "Running node with ID: "
       << node_id() << "\n"
       << " Ethereum Address ID: "
       << ethereum_address() << "\n"
       << "             on port: " << get_port() << "\n"
       << "       Token Balance: " << get_token_balance() << " BLZ\n";
    return ss.str();
}
