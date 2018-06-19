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

#include <utils/http_get.hpp>
#include <bootstrap/bootstrap_peers.hpp>
#include <fstream>
#include <json/json.h>

using namespace bzn;


bool bootstrap_peers::fetch_peers_from_file(const std::string& filename)
{
    std::ifstream file(filename);
    if (file.fail())
    {
        LOG(error) << "Failed to read bootstrap peers file " << filename;
        return false;
    }

    LOG(info) << "Reading peers from " << filename;

    return ingest_json(file);
}


bool 
bootstrap_peers::fetch_peers_from_url(const std::string& url)
{
    std::string peers = bzn::utils::http::sync_get(url);

    LOG(info) << "Downloaded peer list from " << url;

    std::stringstream stream;
    stream << peers;

    return ingest_json(stream);
}


const bzn::peers_list_t&
bootstrap_peers::get_peers() const
{
    return this->peer_addresses;
}


bool 
bootstrap_peers::ingest_json(std::istream& peers)
{
    size_t addresses_before = this->peer_addresses.size();
    size_t valid_addresses_read = 0;

    Json::Value root;

    try
    {
        peers >> root;
    }
    catch (const std::exception& e)
    {
        LOG(error) << "Failed to parse peer JSON (" << e.what() << ")";
        return false;
    }

    // Expect the read json to be an array of peer objects
    for (const auto& peer : root)
    {
        std::string host;
        std::string name;
        std::string uuid;
        uint16_t    port;
        uint16_t    http_port;

        try
        {
            host = peer["host"].asString();
            port = peer["port"].asUInt();
            http_port = peer["http_port"].asUInt();
            uuid = peer.isMember("uuid") ? peer["uuid"].asString() : "unknown";
            name = peer.isMember("name") ? peer["name"].asString() : "unknown";
        }
        catch(std::exception& e)
        {
            LOG(warning) << "Ignoring malformed peer specification " << peer;
            continue;
        }

        // port wasn't actually a 16 bit uint
        if (peer["port"].asUInt() != port)
        {
            LOG(warning) << "Ignoring peer with bad port " << peer;
            continue;
        }

        if (peer["http_port"].asUInt() != http_port)
        {
            LOG(warning) << "Ignoring peer with bad http port " << peer;
            continue;
        }

        // peer didn't contain everything we need
        if (host.empty() || port == 0 || http_port == 0)
        {
            LOG(warning) << "Ignoring underspecified peer (needs host, port and http_port) " << peer;
            continue;
        }

        this->peer_addresses.emplace(host, port, http_port, name, uuid);

        LOG(trace) << "Found peer " << host << ":" << port << " (" << name << ")";

        valid_addresses_read++;
    }

    size_t new_addresses = this->peer_addresses.size() - addresses_before;
    size_t duplicate_addresses = new_addresses - valid_addresses_read;

    LOG(info) << "Found " << new_addresses << " new peers";

    if (duplicate_addresses > 0)
    {
        LOG(info) << "Ignored " << duplicate_addresses << " duplicate addresses";
    }

    return new_addresses > 0;
}
