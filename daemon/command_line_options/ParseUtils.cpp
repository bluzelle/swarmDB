#include "CommandLineOptions.h"
#include "ParseUtils.h"
#include "node/DaemonInfo.h"

#include <iostream>

const uint16_t  k_LOWER_DELAY_BOUND = 10;
const uint16_t  k_UPPER_DELAY_BOUND = 2000;

bool
simulated_delay_values_set(
    const CommandLineOptions& options
)
{
    return !(
        (options.get_simulated_delay_lower() == 0) &&
            (options.get_simulated_delay_upper() == 0)
    );
}


bool
lower_ok(const CommandLineOptions& options)
{
    return options.get_simulated_delay_lower() >= k_LOWER_DELAY_BOUND &&
           options.get_simulated_delay_lower() <= options.get_simulated_delay_upper();
}

bool upper_ok(const CommandLineOptions& options)
{
    return options.get_simulated_delay_upper() <= k_UPPER_DELAY_BOUND &&
           options.get_simulated_delay_lower() <= options.get_simulated_delay_upper();
}

bool
simulated_delay_values_within_bounds(
    const CommandLineOptions& options
)
{
    if(simulated_delay_values_set(options))
        {
        return lower_ok(options) && upper_ok(options);
        }
    return true;
}

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
    DaemonInfo::get_instance().set_value( "ethereum_address", address);

    uint port = options.get_port();
    if (!CommandLineOptions::is_valid_port(port))
        {
        std::cout << port << " is not a valid port. Please pick a port in 49152 - 65535 range" << std::endl;
        return 1;
        }
    DaemonInfo::get_instance().set_value("port",port);
    DaemonInfo::get_instance().set_value("name", "Node_running_on_port_" + std::to_string(port));

    if(simulated_delay_values_set(options))
        {
        if(!simulated_delay_values_within_bounds(options))
            {
            std::cout << "Please ensure that the simulated delay values are valid:\n"
                      << " - the lower delay value must be equal to or greater than 10 ms\n"
                      << " - the upper delay value must be equal to or greater than the lower delay value\n"
                      << " - the upper delay value must be less than or equal to 2000 ms."
                      << std::endl;
            return 1;
            }
        }
    return 0;
}