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

    auto new_peers = this->build_peers_list_from_json(root);

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
peers_beacon::build_peers_list_from_json(const json::Value& root)
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

bool
peers_beacon::fetch_peers_from_file(const std::string& filename)
{
    std::ifstream file(filename);
    if (file.fail())
    {
        LOG(error) << "Failed to read bootstrap peers file " << filename;
        return false;
    }

    LOG(info) << "Reading peers from " << filename;

    return parse_and_save_peers(file);
}


bool
peers_beacon::fetch_peers_from_url(const std::string& url)
{
    std::string peers = bzn::utils::http::sync_req(url);

    LOG(info) << "Downloaded peer list from " << url;

    std::stringstream stream;
    stream << peers;

    return parse_and_save_peers(stream);
}


bool
peers_beacon::fetch_peers_from_esr_contract(const std::string& esr_url, const std::string& esr_address, const bzn::uuid_t& swarm_id)
{
    /* TODO
    auto peer_ids = bzn::utils::esr::get_peer_ids(swarm_id, esr_address, esr_url);
    for (const auto& peer_id : peer_ids)
    {
        bzn::peer_address_t peer_info{bzn::utils::esr::get_peer_info(swarm_id, peer_id, esr_address, esr_url)};
        if (peer_info.host.empty()
            || peer_info.port == 0
            //|| peer_info.name.empty() // is it important that a peer have a name?
            || peer_info.uuid.empty()
                )
        {
            LOG(warning) << "Invalid peer information found in esr contract, ignoring info for peer: " << peer_id << " in swarm: " << swarm_id;
        }
        else
        {
            this->peer_addresses.emplace(peer_info);
        }
    }
    return true;
     */
}
