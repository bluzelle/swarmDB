#include <iostream>
#include <string>

#include <boost/algorithm/hex.hpp>

#include "CommandLineUtilities.h"

using std::string;

static const uint s_address_size = 42; // ("0x2ba35056580b505690c03dfb1df58bc6b6cd9f89").length().


bool handle_command_line(int argc, const char *argv[], boost::program_options::variables_map &vm) {
    boost::program_options::options_description desc("Allowed options: ");

    desc.add_options()
            ("help", "This message")
            ("address", boost::program_options::value<std::string>(),
             "Hexadecimal Ethererum address (Ropsten network)");

    try
        {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        }
    catch (boost::program_options::error& err)
        {
        std::cout << err.what() << std::endl;
        return false;
        }

    boost::program_options::notify(vm);

    if (vm.count("help") || vm.count("address") == 0)
        {
        std::cout << desc << "\n";
        return false;
        }

    // Validate that address is HEX.
    auto addr = vm["address"].as<string>();
    if ((addr.length() == s_address_size) &&
            (addr.substr(0,2) == "0x") ||
            (addr.substr(0,2) == "0X"))
        {
        try
        {
            boost::algorithm::unhex(addr.substr(2, addr.length()-2)); // Skip "0x."
            return true;
        }
        catch(boost::algorithm::hex_decode_error&) { }
        }

    std::cout << addr << " is not a valid hexadecimal address." << std::endl;
    return false;
}

