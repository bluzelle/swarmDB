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

#include <pbft/pbft_configuration.hpp>
#include <map>

namespace bzn
{
    using hash_t = std::string;

    class pbft_config_store
    {
    public:
        // add a new configuration to storage. does not enable it or set it current
        bool add(pbft_configuration::shared_const_ptr config);

        // remove configurations from store older than the one with the given hash
        // Note: this will not remove the currently active configuration
        bool remove_prior_to(const hash_t& hash);

        // get the configuration with the given hash
        pbft_configuration::shared_const_ptr get(const hash_t& hash) const;

        // set the configuration with the given hash to be the currently active one
        bool set_current(const hash_t& hash);

        // returns the currently active configuration
        pbft_configuration::shared_const_ptr current() const;

        // mark a configuration enabled/disabled
        bool enable(const hash_t& hash, bool val = true);

        // returns whether the configuration with the given hash is enabled
        bool is_enabled(const hash_t& hash) const;

        // returns the most recent enabled configuration
        pbft_configuration::shared_const_ptr newest() const;

    private:
        using index_t = uint64_t;
        using config_map = std::map<index_t, std::pair<pbft_configuration::shared_const_ptr, bool>>;
        config_map::const_iterator find_by_hash(hash_t hash) const;

        // a map from the config index to a pair of <config, is_config_enabled>
        config_map configs;
        index_t current_index = 0;
        index_t next_index = 1;
    };
}

