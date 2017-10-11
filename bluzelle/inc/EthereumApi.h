#ifndef KEPLER_ETHEREUMAPI_H
#define KEPLER_ETHEREUMAPI_H

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using std::string;
using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

class EthereumApi {
protected:
    string host = "api.etherscan.io";
    const string getTokenBalanceByTokenContractAddressFormat =
            "/api?module=account&action=tokenbalance&contractaddress=%s&address=%s&tag=latest&apikey=YourApiKeyToken";
    const string httpGetFormat =
            "GET %s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nConnection: close\r\n\r\n";


    string address;

public:
    EthereumApi(const string& a) : address(a) {}

    double tokenBalance(const string& tokenContractAddress);
    double restTest();

private:
    string sendRequest(const string& hst, const string& req);
};


#endif //KEPLER_ETHEREUMAPI_H