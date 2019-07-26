// Copyright (C) 2019 Bluzelle
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

#include <peers_beacon/peers_beacon.hpp>

using namespace bzn;

const std::unordered_set<peer_address_t>&
peers_beacon::current() const
{
    return this->internal_current;
}

bool
peers_beacon::ingest_json(std::istream& source)
{
    Json::Value root;

    try
    {
        peers >> root;
    }
    catch (const std::exception& e)
    {
        LOG(error) << "Failed to parse peer JSON (" << e.what() << ")";
        LOG(error) << "Keeping old peer list";
        return false;
    }

    size_t valid_addresses_read = this->initialize_peer_list(root, this->peer_addresses);

    size_t new_addresses = this->peer_addresses.size() - addresses_before;
    size_t duplicate_addresses = new_addresses - valid_addresses_read;

    LOG(info) << "Found " << new_addresses << " new peers";

    if (duplicate_addresses > 0)
    {
        LOG(warning) << "Ignored " << duplicate_addresses << " duplicate addresses";
    }

    if (new_address == 0)
    {
        LOG(error) << "failed to read any peers";
        LOG(error) << "Keeping old peer list";
    }

    return new_addresses > 0;

}