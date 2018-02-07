#include <iostream>
#include <sstream>

#include <boost/algorithm/hex.hpp>

#include "CommandLineOptions.h"

using namespace boost::program_options;

const uint CommandLineOptions::s_address_size = 42; // ("0x2ba35056580b505690c03dfb1df58bc6b6cd9f89").length().

const string CommandLineOptions::s_help_option_name = "help";
const string CommandLineOptions::s_address_option_name = "address";
const string CommandLineOptions::s_config_option_name = "config";
const string CommandLineOptions::s_port_option_name = "port";

CommandLineOptions::CommandLineOptions()
        : desc_("Allowed options") {

    desc_.add_options()
            ((s_help_option_name + ",h").c_str(), "produce help message")
            ((s_address_option_name + ",a").c_str(), value<string>(&address_)->required(), "Ethererum account address")
            ((s_port_option_name + ",p").c_str(), value<ushort>(&port_)->required(), "port to use")
            ((s_config_option_name + ",c").c_str(), value<string>(&config_), "path to configuration file");
}

bool CommandLineOptions::parse(int argc, char *argv[]) {
    try
        {
        store(parse_command_line(argc,
                                 argv,
                                 desc_), vm_);

        if ((vm_.count(s_help_option_name) != 0) || (argc == 1))
            {
            std::stringstream ss;
            ss << desc_;
            error_ = ss.str();
            return false;
            }

        notify(vm_);
        }
    catch (error &err)
        {
        error_ =  err.what();
        return false;
        }

    return true;
}

string CommandLineOptions::get_last_error() const {
    return error_;
}

options_description CommandLineOptions::get_description() const {
    return desc_;
}

bool CommandLineOptions::is_valid_ethereum_address(const string &addr) {
    if ((addr.length() == s_address_size) &&
            ((addr.substr(0, 2) == "0x") ||
                    (addr.substr(0, 2) == "0X")))
        {
        try
            {
            boost::algorithm::unhex(addr.substr(2, addr.length() - 2)); // Skip "0x."
            return true;
            }
        catch (boost::algorithm::hex_decode_error &)
            {

            }
        }
    return false;
}

bool CommandLineOptions::is_valid_port(ushort p) {
    return (p >= 49152); // As per IANA https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml
}

ushort CommandLineOptions::get_port() const {
    return get_option<ushort>(s_port_option_name);
}

string CommandLineOptions::get_address() const {
    return get_option<string>(s_address_option_name);
}

string CommandLineOptions::get_config() const {
    return get_option<string>(s_config_option_name);
}

