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

#include <pbft/pbft_config_store.hpp>

using namespace bzn;

void
pbft_config_store::add(pbft_configuration::shared_const_ptr config)
{
    auto res = this->configs.insert(std::make_pair(config->get_hash(), config_info{++this->index, config}));
    if (!res.second)
    {
        // reset the configuration info without losing its previous view(s)
        res.first->second.index = this->index;
        res.first->second.state = pbft_config_state::accepted;
    }
}

pbft_configuration::shared_const_ptr
pbft_config_store::get(const hash_t& hash) const
{
    auto it = this->configs.find(hash);
    return it == this->configs.end() ? nullptr : it->second.config;
}

pbft_configuration::shared_const_ptr
pbft_config_store::get(uint64_t view) const
{
    auto it = this->view_configs.find(view);
    return it == this->view_configs.end() ? nullptr : get(it->second);
}

bool
pbft_config_store::set_prepared(const hash_t& hash)
{
    return this->set_state(hash, pbft_config_state::prepared);
}

bool
pbft_config_store::set_committed(const hash_t& hash)
{
    if (this->set_state(hash, pbft_config_state::committed))
    {
        // when a configuration is committed, we won't accept new_view with any earlier config
        for (auto& elem : this->configs)
        {
            if (elem.second.state != pbft_config_state::current && elem.second.index < this->configs[hash].index)
            {
                elem.second.state = pbft_config_state::deprecated;
            }
        }

        return true;
    }

    return false;
}

bool
pbft_config_store::set_current(const hash_t& hash, uint64_t view)
{
    if (this->view_configs.find(view) != this->view_configs.end())
    {
        LOG(error) << "Attempt to set configuration for a view that already has one: " << view;
        return false;
    }

    auto it = this->configs.find(hash);
    if (it != this->configs.end())
    {
        it->second.state = pbft_config_state::current;
        it->second.views.insert(view);
        this->view_configs[view] = hash;

        this->current_config = hash;
        return true;
    }
    return false;
}

pbft_configuration::shared_const_ptr
pbft_config_store::current() const
{
    auto it = this->configs.find(this->current_config);
    return it == this->configs.end() ? nullptr : it->second.config;
}

hash_t
pbft_config_store::newest_prepared() const
{
    return this->newest({pbft_config_state::prepared, pbft_config_state::committed, pbft_config_state::current});
}

hash_t
pbft_config_store::newest_committed() const
{
    return this->newest({pbft_config_state::committed, pbft_config_state::current});
}

pbft_config_store::pbft_config_state
pbft_config_store::get_state(const hash_t& hash) const
{
    auto it = this->configs.find(hash);
    return it == this->configs.end() ? pbft_config_state::unknown : it->second.state;
}

bool pbft_config_store::is_acceptable(const hash_t& hash) const
{
    return this->get_state(hash) != pbft_config_state::unknown && this->get_state(hash) != pbft_config_state::deprecated;
}

bool
pbft_config_store::set_state(const hash_t& hash, pbft_config_state state)
{
    auto it = this->configs.find(hash);
    if (it != this->configs.end())
    {
        it->second.state = state;
        return true;
    }
    return false;
}

hash_t
pbft_config_store::newest(const std::list<pbft_config_state>& states) const
{
    hash_t hash;
    uint64_t max{0};
    for (auto elem : this->configs)
    {
        if (elem.second.index > max && std::any_of(states.begin(), states.end(), [elem](auto state)
        {
            return elem.second.state == state;
        }))
        {
            max = elem.second.index;
            hash = elem.first;
        }
    }

    return hash;
}
