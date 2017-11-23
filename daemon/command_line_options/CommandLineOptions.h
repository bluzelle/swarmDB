#ifndef BLUZELLE_COMMANDLINEOPTIONS_H
#define BLUZELLE_COMMANDLINEOPTIONS_H

#include <string>

using std::string;

#include <boost/program_options.hpp>


class CommandLineOptions {
private:
    static const uint s_address_size;

    static const string s_help_option_name;
    static const string s_address_option_name;
    static const string s_config_option_name;
    static const string s_port_option_name;

    boost::program_options::variables_map vm_;
    boost::program_options::options_description desc_;

    ushort port_;
    string address_;
    string config_;

    template<typename T>
    T get_option(const string &name) const {
        if (vm_.count(name) == 0)
            return T();
        return vm_[name].as<T>();
    }

public:
    CommandLineOptions();

    bool parse(int argc, char *argv[]);

    boost::program_options::options_description get_description() const;

    ushort get_port() const;

    string get_address() const;

    string get_config() const;

    static bool is_valid_ethereum_address(const string &addr);

    static bool is_valid_port(ushort p);
};

#endif //BLUZELLE_COMMANDLINEOPTIONS_H
