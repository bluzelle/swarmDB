#include "CommandLineOptions.h"
#include "Listener.h"
#include "WebSocketServer.h"
#include "TokenBalance.hpp"
#include "node/Singleton.h"
#include "node/DaemonInfo.h"
#include "Node.h"
#include "ParseUtils.h"

#include <boost/uuid/uuid_generators.hpp>
//#include <iostream>
//#include <boost/thread.hpp>

constexpr int s_uint_minimum_required_token_balance = 100;
constexpr char s_etherscan_api_token_envar_name[] = "ETHERSCAN_IO_API_TOKEN";

void initialize_daemon()
{
    // TODO: Mehdi, put your config file work here.
    // If we don't have a node id in the config file, then generate one here
    // Mehdi, for now we just generate a new  node id everytime we restart the node,
    // but we should really check the config file first.
    boost::uuids::basic_random_generator<boost::mt19937> gen;
    const boost::uuids::uuid node_id{gen()};
    DaemonInfo::get_instance().set_value("node_id", boost::lexical_cast<std::string>(node_id));
}

void display_daemon_info()
{
    DaemonInfo& daemon_info = DaemonInfo::get_instance();
    auto port = daemon_info.get_value<unsigned short>("port");

    std::cout << "Running node with ID: " << daemon_info.get_value<std::string>("node_id") << "\n"
              << " Ethereum Address ID: " << daemon_info.get_value<std::string>("ethereum_address") << "\n"
              << "             on port: " << port << "\n"
              << "       Token Balance: " << daemon_info.get_value<unsigned short>("ropsten_token_balance") << " BLZ\n"
              << std::endl;
}

int check_token_balance()
{
    auto token = getenv(s_etherscan_api_token_envar_name);
    if (token == nullptr)
        {
        std::cout << "Environment variable " << s_etherscan_api_token_envar_name
                  << " containing etherscan.io API token must be set" << std::endl;
        return 1;
        }

    DaemonInfo& daemon_info = DaemonInfo::get_instance();

    uint64_t balance = get_token_balance(daemon_info.get_value<std::string>("ethereum_address"), token);
    if (balance < s_uint_minimum_required_token_balance)
        {
        std::cout << "Insufficient BZN token balance. Te balance of "
                  << s_uint_minimum_required_token_balance << "BZN required to run node. Your balance is "
                  << balance << "BZN" << std::endl;
        return 1;
        }
    daemon_info.set_value("ropsten_token_balance", balance);
    return 0;
}

int main(int argc, const char *argv[])
{
    initialize_daemon();

    if( 0 != parse_command_line(argc, argv) )
        {
        return -1;
        }

    if( 0 != check_token_balance())
        {
        return -1;
        }

    display_daemon_info();

    ///////////////////////////////////////////////////////////////////////////
    // Start the daemon work.
    std::shared_ptr<Listener> listener;
    boost::thread websocket_thread
            (
            WebSocketServer
                    (
                            "127.0.0.1",
                            DaemonInfo::get_instance().get_value<unsigned short>("port") + 1000,
                            listener,
                            1
                    )
            );

    boost::asio::io_service ios; // I/O service to use.
    boost::thread_group tg;

    Node this_node(ios);
    try
        {
        this_node.run();
        for (unsigned i = 0; i < boost::thread::hardware_concurrency(); ++i)
            {
            tg.create_thread(boost::bind(&boost::asio::io_service::run, &ios));
            }
        ios.run();
        }
    catch (std::exception& ex)
        {
        std::cout << ex.what() << std::endl;
        return 1;
        }

    return 0;
}
