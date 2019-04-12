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

#include <curl/curl.h>
#include <utils/curl.hpp>

// TODO: remove the dependance on libCurl there is a good example in Pauls github
// repo here https://github.com/paularchard/bzchallenge/blob/master/src/url_reader.cpp
namespace
{
    const std::string MSG_ERROR_CURL{"curl_easy_perform() failed in whitelist: "};

    size_t
    write_function(void* contents,size_t size, size_t nmemb, void* userp)
    {
        reinterpret_cast<std::string*>(userp)->append(reinterpret_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }

}

namespace bzn::utils::curl
{
    std::string
    perform_curl_request(const std::string& url, const std::string& request)
    {
        std::string readBuffer;
        std::unique_ptr<CURL, std::function<void(CURL*)>> curl(curl_easy_init(), std::bind(curl_easy_cleanup, std::placeholders::_1));
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
        const std::string post_fields{request};
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, post_fields.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, -1L);
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, reinterpret_cast<void*>(write_function));

        CURLcode res = curl_easy_perform(curl.get());
        if (res != CURLE_OK)
        {
            throw (std::runtime_error(MSG_ERROR_CURL + curl_easy_strerror(res)));
        }
        return readBuffer;
    }
}
