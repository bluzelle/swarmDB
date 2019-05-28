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

#include <include/bluzelle.hpp>
//#include <utils/curl.hpp>
#include <utils/http_req.hpp>
#include <boost/format.hpp>

namespace
{
    const std::string ROPSTEN_REQUEST_DATA{R"({"jsonrpc":"2.0","method":"eth_call","params":[{"to":"0x58261EEc3fCD83DACB5E0532277c27f1cA58270E","data": "0x3a4e44a0%s" },"latest"],"id":1})"};
    const std::string ERR_INVALID_RESPONSE{"Invalid response from Bluzelle blacklist server: "};

    bzn::uuid_t
    clean_uuid(const bzn::uuid_t& uuid)
    {
        bzn::uuid_t cleaned_uuid;
        std::copy_if(uuid.begin(), uuid.end(), std::back_inserter(cleaned_uuid), [](auto c) { return c != '-'; });
        return cleaned_uuid;
    }
}


namespace bzn::utils::blacklist
{
    bool
    is_blacklisted(const bzn::uuid_t& raw_uuid, const std::string& url)
    {
        // We get the uuid in the following format: "9dc2f619-2e77-49f7-9b20-5b55fd87ea44", the contract expects a
        // string of 32 hex values, so let's at least remove the dashes.
        const std::string post_fields{boost::str(boost::format(ROPSTEN_REQUEST_DATA) % clean_uuid(raw_uuid))};
        bzn::json_message response;
        Json::Reader reader;

        //if (!reader.parse(bzn::utils::curl::perform_curl_request(url.c_str(), post_fields), response))
        if (!reader.parse(bzn::utils::http::sync_req(url, post_fields), response))
        {
            LOG(error) << "Unable to parse response from Ropsten - could not validate peer";
            return false;
        }

        try
        {
            // The result must be "0x0000000000000000000000000000000000000000000000000000000000000000" or
            // "0x0000000000000000000000000000000000000000000000000000000000000001" we will only accept a value of 1 as
            // true.
            return (std::stoul(response["result"].asString().c_str(), nullptr, 16) == 1);
        }
        catch(std::exception& ex)
        {
            LOG(error) << ERR_INVALID_RESPONSE << ex.what();
        }
        return false;
    }
}
