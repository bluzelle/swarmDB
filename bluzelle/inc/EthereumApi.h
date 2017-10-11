#ifndef KEPLER_ETHEREUMAPI_H
#define KEPLER_ETHEREUMAPI_H

#include <string>

using std::string;

class EthereumApi {
protected:
    const string host = "api.etherscan.io";
    const string httpGetFormat =
            "GET %s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nConnection: close\r\n\r\n";
    const string getTokenBalanceByTokenContractAddressFormat =
            "/api?module=account&action=tokenbalance&contractaddress=%s&address=%s&tag=latest&apikey=YourApiKeyToken";

    string address;

public:
    EthereumApi(const string& a) : address(a) {}

    double tokenBalance(const string& tokenContractAddress);

private:
    static string sendRequest(const string& hst, const string& req);
};





#endif //KEPLER_ETHEREUMAPI_H