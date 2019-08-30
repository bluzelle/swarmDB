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
#include <options/options_base.hpp>
#include <peers_beacon/peers_beacon_base.hpp>
#include <peers_beacon/peer_address.hpp>

namespace bzn
{
    class peers_beacon : peers_beacon_base, public std::enable_shared_from_this<peers_beacon>
    {
    public:
        peers_beacon(std::shared_ptr<bzn::options_base> opt);

        // do an initial pull and start any necessary timers
        void start() override;

        // get a pointer to the current peers list (which won't change, but can be replaced at any time)
        std::shared_ptr<const peers_list_t> current() const override;

        std::shared_ptr<const ordered_peers_list_t> ordered() const override;

        // refresh from whatever source we are using
        bool refresh(bool first_run = false) override;

    private:

        bool fetch_from_esr();
        bool fetch_from_file();
        bool fetch_from_url();

        bool parse_and_save_peers(std::istream& source);
        peers_list_t build_peers_list_from_json(const Json::Value& root);

        bool switch_peers_list(const peers_list_t& new_peers);

        void run_timer();

        std::shared_ptr<bzn::options_base> options;

        // this is kept as a shared ptr to avoid issues when a reader reads while the peers list changes
        std::shared_ptr<const peers_list_t> internal_current;
        std::shared_ptr<const ordered_peers_list_t> internal_current_ordered;

        std::unique_ptr<bzn::asio::steady_timer_base> refresh_timer;
    };
}

