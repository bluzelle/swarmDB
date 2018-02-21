#pragma once

#include <string>
#include <boost/program_options.hpp>


class CommandLineOptions
{
public:
    CommandLineOptions();

    bool parse(int argc, const char *argv[]);

    std::string get_last_error() const;

    boost::program_options::options_description get_description() const;

    ushort get_port() const;

    std::string get_address() const;

    std::string get_config() const;

    std::string get_host_ip() const;

    static bool is_valid_ethereum_address(const std::string& addr);

    static bool is_valid_port(ushort p);

private:
    static const uint s_address_size;

    static const std::string s_help_option_name;
    static const std::string s_address_option_name;
    static const std::string s_host_ip_option_name;
    static const std::string s_config_option_name;
    static const std::string s_port_option_name;

    boost::program_options::variables_map vm_;
    boost::program_options::options_description desc_;

    ushort port_;
    std::string address_;
    std::string host_ip_;
    std::string config_;
    std::string error_;

    template<typename T>
    T get_option(const std::string& name) const {
        if (vm_.count(name) == 0)
            return T();
        return vm_[name].as<T>();
    }
};
