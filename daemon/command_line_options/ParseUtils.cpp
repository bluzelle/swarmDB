#include "CommandLineOptions.h"
#include "ParseUtils.h"
#include "node/DaemonInfo.h"

#include <iostream>

int
parse_command_line(
    int argc,
    const char *argv[]
)
{
    CommandLineOptions options;

    if (!options.parse(argc, argv))
        {
        std::cout << options.get_last_error() << std::endl;
        return 1;
        }

    string address = options.get_address();
    if (!CommandLineOptions::is_valid_ethereum_address(address))
        {
        std::cout << address << " is not a valid Ethereum address." << std::endl;
        return 1;
        }
    DaemonInfo::get_instance().ethereum_address() = address ;

    uint16_t port = options.get_port();
    if (!CommandLineOptions::is_valid_port(port))
        {
        std::cout << port << " is not a valid port. Please pick a port in 49152 - 65535 range" << std::endl;
        return 1;
        }
    DaemonInfo::get_instance().host_port() = port;
    DaemonInfo::get_instance().host_name() = "Node_running_on_port_" + std::to_string(port);

    // optional server ip to use...
    DaemonInfo::get_instance().host_ip() = options.get_host_ip();
    return 0;
}
