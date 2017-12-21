#ifndef BLUZELLE_TOKENBALANCE_H
#define BLUZELLE_TOKENBALANCE_H

#include <string>

#include "EthereumApi.h"
#include "EthereumToken.h"

 // BZN token.
 constexpr int s_uint_bzn_token_decimal_digits = 18;

 constexpr char s_str_bzn_token_name[] = "BZN";
 constexpr char s_str_bzn_token_contract_address[] = "0x2ba35056580b505690c03dfb1df58bc6b6cd9f89";


 uint64_t get_token_balance(const string& address, const string& token) {
     boost::asio::io_service ios;
     EthereumApi api(address, token, ios);

     auto balance = api.token_balance(EthereumToken(s_str_bzn_token_contract_address,
                                                    s_uint_bzn_token_decimal_digits));

     return balance;
 }

#endif //BLUZELLE_TOKENBALANCE_H
