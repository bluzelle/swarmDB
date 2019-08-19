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
#include <peers_beacon/peer_address.hpp>

namespace bzn
{
    using peers_list_t = std::unordered_set<bzn::peer_address_t>;

    class peers_beacon
    {
    public:
        peers_beacon(std::shared_ptr<bzn::options_base> opt);

        // do an initial pull and start any necessary timers
        void start() = 0;

        // get a pointer to the current peers list (which won't change, but can be replaced at any time)
        std::shared_ptr<const peers_list_t> current() const;

        // refresh from whatever source we are using
        virtual bool force_refresh() = 0;

        virtual ~peers_beacon();

    protected:

        bool parse_and_save_peers(std::istream& source);
        peers_list_t build_peers_list_from_json(const Json::Value& root);

        std::shared_ptr<bzn::options_base> options;

        // this is kept as a shared ptr to avoid issues when a reader reads while the peers list changes
        std::atomic<std::shared_ptr<const peers_list_t>> internal_current;
    };
}

