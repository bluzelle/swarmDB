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

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/uuid/uuid.hpp>
#include <functional>
#include <string_view>
#include <json/json.h>


namespace bzn
{
    using msg = Json::Value;

    constexpr std::string_view BLUZELLE_VERSION = "0.0.1";

    using uuid_t = boost::uuids::uuid;

    /*
    using node_info = struct
    {
        std::string host;
        uint16_t    port;
        std::string name;

        boost::uuids::uuid id;
        std::string ethereum_address;
        uint64_t    ropsten_token_balance;
    };
    */

    // forward declare...
    class session_base;

    using msg_handler = std::function<void(const Json::Value& msg, std::shared_ptr<bzn::session_base> session)>;

} // bzn


namespace bzn::utils
{
    constexpr std::string_view basename(const std::string_view& path)
    {
        return path.substr(path.rfind('/') + 1);
    }

} // bzn::utils


// logging
#define LOG(x) BOOST_LOG_TRIVIAL(x) << "(" << bzn::utils::basename(__FILE__) << ":"  << __LINE__ << ") - "
