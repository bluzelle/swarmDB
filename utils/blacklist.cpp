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
#include <boost/format.hpp>
#include <curl/curl.h>

namespace
{
    const std::string ROPSTEN_DATA{R"({"jsonrpc":"2.0","method":"eth_call","params":[{"to":"0x58261EEc3fCD83DACB5E0532277c27f1cA58270E","data": "0x3a4e44a0%s" },"latest"],"id":1})"};
    const std::string MSG_ERROR_CURL{"curl_easy_perform() failed in whitelist: "};

    size_t
    write_function(void* contents,size_t size, size_t nmemb, void* userp)
    {
        reinterpret_cast<std::string*>(userp)->append(reinterpret_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }
}


namespace bzn::utils::blacklist
{
    bool
    is_blacklisted(const bzn::uuid_t& raw_uuid, const std::string& url)
    {
        // We get the uuid in the following format:
        //      "9dc2f619-2e77-49f7-9b20-5b55fd87ea44",
        // the contract expects a string of 32 hex values, so let's at least
        // remove the dashes.
        std::string uuid;
        std::copy_if(raw_uuid.begin(), raw_uuid.end(), std::back_inserter(uuid), [](auto c) { return c != '-'; });

        assert(32 == uuid.size()); // debug only - ensure that "cleaned" uuid contains only 32 characters.
        // TODO: rhn - is it worth it to ensure that uuid contains only hex?

        std::unique_ptr<CURL, std::function<void(CURL*)>> curl(curl_easy_init(),
                                                               std::bind(curl_easy_cleanup, std::placeholders::_1));

        std::string readBuffer;
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);

        const std::string post_fields{boost::str(boost::format(ROPSTEN_DATA) % uuid)};
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, post_fields.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, -1L);
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, reinterpret_cast<void*>(write_function));

        CURLcode res = curl_easy_perform(curl.get());
        if (res != CURLE_OK)
        {
            throw (std::runtime_error(MSG_ERROR_CURL + curl_easy_strerror(res)));
        }

        bzn::json_message response;
        Json::Reader reader;

        if (!reader.parse(readBuffer, response))
        {
            LOG(error) << "Unable to parse response from Ropsten - could not validate peer";
            return false;
        }

        try
        {
            // result must be "0x0000000000000000000000000000000000000000000000000000000000000000"
            // or "0x0000000000000000000000000000000000000000000000000000000000000001" we will only
            // accept a value of 1 as true.
            return (std::stoul(response["result"].asString().c_str(), nullptr, 16) == 1);
        }
        catch(std::exception& ex)
        {
            LOG(error) << "Invalid response from Bluzelle blacklist server: " << ex.what();
        }
        return false;
    }
}
