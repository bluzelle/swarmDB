#ifndef KEPLER_ETHEREUMTOKEN_H
#define KEPLER_ETHEREUMTOKEN_H

#include <string>

class EthereumToken {
public:
    std::string address_;
    unsigned int decimal_points_;

    EthereumToken(std::string str_address,
            unsigned int uint_decimal_points)
            : address_(str_address), decimal_points_(uint_decimal_points) { }
};

#endif //KEPLER_ETHEREUMTOKEN_H
