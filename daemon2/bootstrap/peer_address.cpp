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
#include <bootstrap/peer_address.hpp>

#include <boost/functional/hash.hpp>

using namespace bzn;

bool
peer_address::operator==(const peer_address& other) const
{
    return this->host == other.host && this->port == other.port;
}

std::size_t bzn::hash_value(const peer_address& item)
{
    std::size_t seed = 0;
    boost::hash_combine(seed, item.host);
    boost::hash_combine(seed, item.port);

    return seed;
}
