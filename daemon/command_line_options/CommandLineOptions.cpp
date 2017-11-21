#include "CommandLineOptions.h"

#include <boost/algorithm/hex.hpp>

using namespace boost::program_options;

static const uint s_address_size = 42; // ("0x2ba35056580b505690c03dfb1df58bc6b6cd9f89").length().

CommandLineOptions::CommandLineOptions()
        : desc_("Allowed options: ") {

}

void CommandLineOptions::parse(int argc, char *argv[]) {
    desc_.add_options()
            ("help", "This message")
            ("address,a", value<string>()->required(), "Ethererum account address"),
            ("port,p", value<ushort>()->required(), "Port to use"),
            ("config,c", value<string>(), "Path to configuration file");

    try
        {
        store(parse_command_line(argc,
                                 argv,
                                 desc_), vm_);
        }
    catch (error &err)
        {}

    notify(vm_);
}

options_description CommandLineOptions::get_description() const {
    return desc_;
}

template<typename T>
T CommandLineOptions::get_option(const string &name) const {
    auto o = vm_[name].as<T>();
    return o;
}

bool CommandLineOptions::is_valid_ethereum_address(const string &addr) {
    if ((addr.length() == s_address_size) &&
        (addr.substr(0, 2) == "0x") ||
        (addr.substr(0, 2) == "0X"))
        {
        try
            {
            boost::algorithm::unhex(addr.substr(2, addr.length() - 2)); // Skip "0x."
            return true;
            }
        catch (boost::algorithm::hex_decode_error &)
            {}
        }
    return false;
}

bool CommandLineOptions::is_valid_port(ushort p) {
    return p >= 49152 && p <= 65535;
}

