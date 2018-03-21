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
#include <unordered_set>

#include <boost/functional/hash.hpp>

#include <include/bluzelle.hpp>
#include <bootstrap/peer_address.hpp>

namespace bzn
{
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
         * @return a reference to the initial set of peers
         */
        virtual const std::unordered_set<peer_address, boost::hash<peer_address>>& get_peers() const = 0; 
    };
}
