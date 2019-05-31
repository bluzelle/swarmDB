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

#include <include/bluzelle.hpp>
#include <bootstrap/peer_address.hpp>
#include <unordered_set>


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

namespace bzn
{
    using peers_list_t = std::unordered_set<bzn::peer_address_t>;

    class bootstrap_peers_base
    {
    public:

        /**
         * Read and parse peers specified in a local file (existing peers will
         * not be overwritten). 
         * @return if at least one peer fetched successfully
         */
        virtual bool fetch_peers_from_file(const std::string& filename) = 0;

        /**
         * Fetch and parse peers specified in json from some url (existing
         * peers will not be overwritten).
         * @return if at least one peer fetched successfully
         */
        virtual bool fetch_peers_from_url(const std::string& url) = 0;

        /**
         * Given a swarm id, fetch the peer info for the peers in that swarm
         * @param esr_url a string containing the url of the Etherium contract server
         * @param esr_address a string containing the Etherium address of the get peer info contract
         * @param swarm_id a string containing the unique identifier of the swarm containing the peers of interest
         * @return true - note that it is possible that the contract does not return any peers
         */
        virtual bool fetch_peers_from_esr_contract(const std::string &esr_url, const std::string &esr_address, const bzn::uuid_t &swarm_id) = 0;

        /**
         * @return a reference to the initial set of peers
         */
        virtual const peers_list_t& get_peers() const = 0;
    };

} // namespace bzn
