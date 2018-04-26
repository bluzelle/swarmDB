#ifndef BLUZELLE_ETHEREUMTOKEN_H
#define BLUZELLE_ETHEREUMTOKEN_H

#include <string>

class EthereumToken {
    std::string address_;
    unsigned int decimal_points_;

public:
    EthereumToken(std::string str_address,
                  unsigned int uint_decimal_points)
            : address_(str_address), decimal_points_(uint_decimal_points) { }

    std::string get_address() const {
        return address_;
    }

    unsigned int get_decimal_points() const {
        return decimal_points_;
    }
};

#endif //BLUZELLE_ETHEREUMTOKEN_H
