#ifndef BLUZELLE_PARSEUTILS_H
#define BLUZELLE_PARSEUTILS_H

bool
simulated_delay_values_set(
    const CommandLineOptions& options
);

bool
simulated_delay_values_within_bounds(
    const CommandLineOptions& options
);

int
parse_command_line(
    int argc,
    const char *argv[]
);

#endif //BLUZELLE_PARSEUTILS_H
