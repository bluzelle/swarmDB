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

#pragma once

#include <boost/log/trivial.hpp>
#include <string_view>
#include <json/json.h>
#include <swarm_version.hpp>


namespace bzn
{
    using caller_id_t = std::string;

    using encoded_message = std::string;

    using hash_t = std::string;

    using json_message = Json::Value;

    using key_t = std::string;

    using service_state_t = std::string;

    using session_id = uint64_t;

    using uuid_t = std::string;

    using value_t = std::string;

} // bzn


namespace bzn::utils
{
    constexpr
    std::string_view basename(const std::string_view& path)
    {
        return path.substr(path.rfind('/') + 1);
    }
} // bzn::utils


// logging
#define LOG(x) BOOST_LOG_TRIVIAL(x) << "(" << bzn::utils::basename(__FILE__) << ":"  << __LINE__ << ") - "

// This limits the number of characters that are displayed in "LOG(x) <<"  messages.
const uint16_t MAX_MESSAGE_SIZE = 1024;
const uint16_t MAX_SHORT_MESSAGE_SIZE = 32;
