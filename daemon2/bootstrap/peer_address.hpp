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

#include <boost/functional/hash.hpp>

namespace bzn
{
    struct peer_address
    {
        const std::string host;
        const uint16_t port;
        const std::string name;

        peer_address(const std::string& host, uint16_t port, const std::string& name) : 
            host(host),
            port(port),
            name(name){};

        bool operator==(const peer_address& other) const;
    };

    std::size_t hash_value(const peer_address& item);
}
