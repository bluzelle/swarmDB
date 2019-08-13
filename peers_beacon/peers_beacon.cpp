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

peers_beacon::peers_beacon(std::shared_ptr<bzn::options_base> opt)
        : options(opt)
{}

void
peers_beacon::start()
{
    this->force_refresh();
    throw std::runtime_error("todo: timer");
}

const std::shared_ptr<const peers_list_t>
peers_beacon::current() const
{
    return this->internal_current;
}

bool
peers_beacon::parse_and_save_peers(std::istream& source)
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

    auto new_peers = this->build_peers_list(root);

    if (new_peers.size() == 0)
    {
        LOG(error) << "Failed to read any peers";
        if (this->internal_current->size() > 0)
        {
            LOG(error) << "Keeping old peer list";
        }
        else
        {
            LOG(error) << "Old peers list also empty";
            throw std::runtime_error("failed to find a valid peers list");
        }
        return false;
    }

    this->internal_current = std::make_shared<const peers_list_t>(new_peers);
    return true;
}

peers_list_t
peers_beacon::build_peers_list(const json::Value& root)
{
    peers_list_t result;

    // Expect the read json to be an array of peer objects
    for (const auto& peer : root)
    {
        std::string host;
        std::string name;
        std::string uuid;
        uint16_t    port;

        try
        {
            host = peer["host"].asString();
            port = peer["port"].asUInt();
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

        // peer didn't contain everything we need
        if (host.empty() || port == 0)
        {
            LOG(warning) << "Ignoring underspecified peer (needs host and port) " << peer;
            continue;
        }

        LOG(trace) << "Found peer " << host << ":" << port << " (" << name << ")";

        result.emplace(host, port, name, uuid);
    }

    LOG(trace) << "Found " << result.size() << " well formed peers";
    return result;
}