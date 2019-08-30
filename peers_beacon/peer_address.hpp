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

#include <string>


namespace bzn
{
    struct peer_address_t
    {
        peer_address_t(std::string host, uint16_t port, std::string name, std::string uuid)
            : host(std::move(host))
            , port(port)
            , name(std::move(name))
            , uuid(std::move(uuid))
        {
        };

        bool operator==(const peer_address_t& other) const
        {
            if (&other == this)
            {
                return true;
            }

            return this->host == other.host && this->port == other.port && this->uuid == other.uuid;
        }

        std::string host;
        uint16_t    port;
        std::string name;
        std::string uuid;
    };
}

namespace std
{
    template<> struct hash<bzn::peer_address_t>
    {
        size_t operator()(const bzn::peer_address_t& x) const noexcept
        {
            return std::hash<std::string>()(std::string{x.host} + std::to_string(x.port));
        }
    };
}
