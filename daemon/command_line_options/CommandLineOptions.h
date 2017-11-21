#ifndef BLUZELLE_COMMANDLINEOPTIONS_H
#define BLUZELLE_COMMANDLINEOPTIONS_H

#include <string>

using std::string;

#include <boost/program_options.hpp>


class CommandLineOptions {
private:
    boost::program_options::variables_map vm_;
    boost::program_options::options_description desc_;

    static const uint s_address_size;

public:
    CommandLineOptions();

    void parse(int argc, char *argv[]);

    boost::program_options::options_description get_description() const;

    template<typename T>
    T get_option(const string &name) const;

    static bool is_valid_ethereum_address(const string& addr);
    static bool is_valid_port(ushort p);
};

#endif //BLUZELLE_COMMANDLINEOPTIONS_H
