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

#pragma once
#include <include/bluzelle.hpp>
#include <string>
#include <vector>
#include <peers_beacon/peer_address.hpp>


namespace bzn
{
    class utils_interface_base
    {
    public:
        virtual std::vector<std::string> get_peer_ids(const bzn::uuid_t& swarm_id, const std::string& esr_address, const std::string& url) const = 0;
        virtual bzn::peer_address_t get_peer_info(const bzn::uuid_t& swarm_id, const std::string& peer_id, const std::string& esr_address, const std::string& url) const = 0;

        // Performs an HTTP GET or POST and returns the body of the HTTP response
        virtual std::string sync_req(const std::string& url, const std::string& post = "") const = 0;

        virtual ~utils_interface_base() = default;
    };
}

