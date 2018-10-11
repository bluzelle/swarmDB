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

#include "pbft_configuration.hpp"
#include <cstdint>
#include <memory>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <functional>

using namespace bzn;

pbft_configuration::pbft_configuration()
{
    this->sorted_peers = std::make_shared<std::vector<bzn::peer_address_t>>();
}

bool
pbft_configuration::operator==(const pbft_configuration& other) const
{
    return this->get_hash() == other.get_hash();
}

bool
pbft_configuration::operator!=(const pbft_configuration& other) const
{
    return !(this->operator==(other));
}

bool
pbft_configuration::from_json(const bzn::json_message& json)
{
    if (!json.isMember("peers") || !json["peers"].isArray())
    {
        LOG(error) << "Invalid configuration: " << json.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";
        return false;
    }

    bool result = true;
    this->peers.clear();
    for (auto p : json["peers"])
    {
        bzn::peer_address_t peer(p["host"].asString(), static_cast<uint16_t>(p["port"].asUInt()),
            static_cast<uint16_t>(p["http_port"].asUInt()), p["name"].asString(), p["uuid"].asString());

        if (this->conflicting_peer_exists(peer) || !this->valid_peer(peer))
        {
            LOG(warning) << "Attempt to add conflicting or invalid peer: "
                         << json.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";
            result = false;
        }
        else
        {
            this->peers.insert(peer);
        }
    }

    this->cache_sorted_peers();
    return result;
}

bzn::json_message
pbft_configuration::to_json() const
{
    bzn::json_message json;
    json["peers"] = bzn::json_message();
    for(const auto& p : *(this->sorted_peers))
    {
        bzn::json_message peer;
        peer["host"] = p.host;
        peer["port"] = p.port;
        peer["http_port"] = p.http_port;
        peer["name"] = p.name;
        peer["uuid"] = p.uuid;

        json["peers"].append(peer);
    }

    return json;
}

hash_t
pbft_configuration::create_hash() const
{
    // TODO: better hash function

    auto json = this->to_json().toStyledString();
    size_t h = std::hash<std::string>{}(json);
    return std::to_string(h);
}

hash_t
pbft_configuration::get_hash() const
{
    return cached_hash;
}

std::shared_ptr<const std::vector<bzn::peer_address_t>>
pbft_configuration::get_peers() const
{
    return this->sorted_peers;
}

bool
pbft_configuration::insert_peer(const bzn::peer_address_t& peer)
{
    if (this->conflicting_peer_exists(peer) || !this->valid_peer(peer))
    {
        return false;
    }

    this->peers.insert(peer);
    return true;
}

bool
pbft_configuration::add_peer(const bzn::peer_address_t& peer)
{
    if (!this->insert_peer(peer))
    {
        return false;
    }

    this->cache_sorted_peers();
    return true;
}

bool
pbft_configuration::remove_peer(const bzn::peer_address_t& peer)
{
    auto p = this->peers.find(peer);
    if (p != this->peers.end())
    {
        this->peers.erase(p);
        this->cache_sorted_peers();
        return true;
    }

    return false;
}

void
pbft_configuration::cache_sorted_peers()
{
    // peer_address_t contains const members so sorting requires some juggling.
    // first copy peers into a vector so we can access them via an index
    std::vector<peer_address_t> unordered_peers_list;
    std::copy(this->peers.begin(), this->peers.end(), std::back_inserter(unordered_peers_list));

    // now create a vector of indices from 1..n
    std::vector<size_t> indicies(peers.size());
    std::iota(indicies.begin(), indicies.end(), 0);

    // now sort the indices based on the peer uuids in the vector
    std::sort(indicies.begin(), indicies.end(),
        [&unordered_peers_list](const auto& i1, const auto& i2)
        {
          return unordered_peers_list[i1].uuid < unordered_peers_list[i2].uuid;
        }
    );

    // and finally get each peer sequentially by sorted index and add to the destination vector
    auto sorted = std::make_shared<std::vector<bzn::peer_address_t>>();
    std::transform(indicies.begin(), indicies.end(), std::back_inserter(*sorted),
        [&unordered_peers_list](auto& peer_index)
        {
           return unordered_peers_list[peer_index];
        }
    );

    this->sorted_peers = sorted;
    this->cached_hash = this->create_hash();
}

bool
pbft_configuration::conflicting_peer_exists(const bzn::peer_address_t &peer) const
{
    for (auto p : this->peers)
    {
        if (p.uuid == peer.uuid || p.name == peer.name)
            return true;

        if (p.host == peer.host)
        {
            if (p.port == peer.port || p.http_port == peer.http_port)
                return true;
        }
    }

    return false;
}

bool
pbft_configuration::valid_peer(const bzn::peer_address_t &peer) const
{
    if (peer.name.empty() || peer.uuid.empty() || peer.host.empty() || !peer.port || !peer.http_port)
        return false;

    // TODO: validate host address?

    return true;
}
