#ifndef KEPLER_COMMANDLINEUTILITIES_H
#define KEPLER_COMMANDLINEUTILITIES_H

#include <boost/program_options.hpp>

bool handle_command_line(int argc, const char *argv[], boost::program_options::variables_map &vm);

#endif //KEPLER_COMMANDLINEUTILITIES_H
