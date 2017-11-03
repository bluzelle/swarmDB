#include <cstdarg>
#include <vector>
#include <iostream>


#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include "EthereumApi.h"

using std::vector;

double EthereumApi::tokenBalance(const string& tokenContractAddress) {
    auto rest = boost::format(getTokenBalanceByTokenContractAddressFormat) % tokenContractAddress % address;
    auto request = boost::format(httpGetFormat) % rest % host;

    auto response = HttpRequest::send(host, boost::str(request));
    HttpResponse r(response);

    double balance  = -1.0; // Negative balance indicates error.

    if (r.getStatus() != 200)
        return balance;

    auto body = r.getContent();
    if (!body.empty() && (body.front() == '{' && body.back() == '}')) {
        boost::property_tree::ptree tuple;
        std::stringstream ss;
        ss << body;
        boost::property_tree::read_json(ss, tuple);

        auto result_s = tuple.get<string>("result");
        auto message_s = tuple.get<string>("message");
        if (message_s == "OK")
            return boost::lexical_cast<double>(result_s);
    }

    return -1.0; // Negative balance indicates error.
}

