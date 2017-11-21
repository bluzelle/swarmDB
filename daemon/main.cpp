#include <iostream>

#include "command_line_options/CommandLineOptions.h"


int main(int argc, char *argv[]) {
    CommandLineOptions options;

    options.parse(argc, argv);

    if (!options.get_option<string>("help").empty())
        {
        std::cout << options.get_description() << std::endl;
        return 1;
        }

    string address = options.get_option<string>("address");
    if (!CommandLineOptions::is_valid_ethereum_address(address))
        {
        std::cout << address << " is not a valid Ethereum address." << std::endl;
        return 1;
        }

    uint port = options.get_option<uint>("port");
    if (!CommandLineOptions::is_valid_port(port))
        {
        std::cout << port << " is not a valid port. Please pick a port in 49152 - 65535 range" << std::endl;
        return 1;
        }

    return 0;
}