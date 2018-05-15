// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License, version 3,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <ethereum/ethereum.hpp>
#include <include/http_get.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>


namespace
{
    const std::string  GET_ETHER_BALANCE_API{"http://api.etherscan.io/api?module=account&action=balance&address=%s&tag=latest&apikey=%s"};
}

using namespace bzn;


double
ethereum::get_ether_balance(const std::string& address, const std::string& api_key)
{
    std::string response = http::sync_get(boost::str(boost::format(GET_ETHER_BALANCE_API) % address % api_key));

    LOG(debug) << "received response: " << response;

    Json::Reader reader;
    Json::Value  msg;

    if (!reader.parse(response, msg))
    {
        LOG(error) << "failed to parse: " << reader.getFormattedErrorMessages();
        return 0;
    }

    // todo: right now we don't care if address is valid or not. Any error and the balance will be zero.
    // Just return 0 for failure as well as no balance.

    double balance = boost::lexical_cast<double>(msg["result"].asString());

    return balance /= 1e18;
}
