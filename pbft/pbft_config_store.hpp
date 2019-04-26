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
#include <pbft/pbft_persistent_state.hpp>
#include <storage/storage_base.hpp>
#include <gtest/gtest_prod.h>
#include <list>
#include <mutex>
#include <map>


namespace
{
    std::string CONFIG_STORE_CONFIGS_KEY{"config_store_configs"};
    std::string CONFIG_STORE_VIEW_CONFIGS_KEY{"config_store_view_configs"};
    std::string CONFIG_STORE_CURRENT_CONFIG_KEY{"config_store_current_config"};
    std::string CONFIG_STORE_INDEX_KEY{"config_store_index"};
}

namespace bzn
{
    using hash_t = std::string;

    class pbft_config_store
    {
    public:
        pbft_config_store(std::shared_ptr<bzn::storage_base> storage);

        // add a new accepted configuration to storage
        void add(pbft_configuration::shared_const_ptr config);

        // get the configuration with the given hash
        pbft_configuration::shared_const_ptr get(const hash_t& hash) const;

        // get the configuration applicable to the given view number (could be archived)
        pbft_configuration::shared_const_ptr get(uint64_t view) const;

        // set the configuration with the given hash to be prepared
        bool set_prepared(const hash_t& hash);

        // set the configuration with the given hash to be committed
        bool set_committed(const hash_t& hash);

        // set the configuration with the given hash to be the currently active one
        bool set_current(const hash_t& hash, uint64_t view);

        // get the configuration marked as current
        pbft_configuration::shared_const_ptr current() const;

        // returns the most recent configuration that is prepared, committed or current
        hash_t newest_prepared() const;

        // returns the most recent configuration that is committed or current
        hash_t newest_committed() const;

        // is the given configuration in a state that can be accepted?
        bool is_acceptable(const hash_t& hash) const;

        enum class pbft_config_state {unknown, accepted, prepared, committed, current, deprecated};

        struct config_info
        {
            uint64_t index = 0;
            pbft_configuration::shared_const_ptr config;
            pbft_config_state state = pbft_config_state::accepted;
            std::set<uint64_t> views{};
        };

    private:
        std::shared_ptr<bzn::storage_base> storage;
        pbft_config_state get_state(const hash_t& hash) const;
        bool set_state(const hash_t& hash, pbft_config_state state);
        hash_t newest(const std::list<pbft_config_state>& states) const;
        pbft_configuration::shared_const_ptr private_get(const hash_t& hash) const;

        std::map<hash_t, persistent<config_info>> configs;
        std::map<uint64_t, persistent<hash_t>> view_configs;
        persistent<hash_t> current_config{storage, "", CONFIG_STORE_CURRENT_CONFIG_KEY};
        persistent<uint64_t> index {storage, 0, CONFIG_STORE_INDEX_KEY};

        mutable std::mutex lock;

        FRIEND_TEST(pbft_config_store_test, state_test);
    };
}

