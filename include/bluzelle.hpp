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

#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)

namespace bzn
{
    using message = Json::Value;

    using uuid_t = std::string;

} // bzn


namespace bzn::utils
{
#if __APPLE__ || GCC_VERSION > 70200
    constexpr
#else
    inline
#endif
    std::string_view basename(const std::string_view& path)
    {
        return path.substr(path.rfind('/') + 1);
    }
} // bzn::utils


// logging
#define LOG(x) BOOST_LOG_TRIVIAL(x) << "(" << bzn::utils::basename(__FILE__) << ":"  << __LINE__ << ") - "
const uint16_t MAX_MESSAGE_SIZE = 1024;