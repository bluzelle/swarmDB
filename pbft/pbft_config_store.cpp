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

pbft_config_store::pbft_config_store(std::shared_ptr<bzn::storage_base> storage)
: storage(storage)
{
    persistent<config_info>::init_kv_container<hash_t>(this->storage, CONFIG_STORE_CONFIGS_KEY, this->configs);
    persistent<hash_t>::init_kv_container<uint64_t>(this->storage, CONFIG_STORE_VIEW_CONFIGS_KEY, this->view_configs);
}

void
pbft_config_store::add(pbft_configuration::shared_const_ptr config)
{
    std::lock_guard<std::mutex> lock(this->lock);
    this->index = this->index.value() + 1;
    auto res = this->configs.insert({config->get_hash()
        , {this->storage, {this->index.value(), config}, CONFIG_STORE_CONFIGS_KEY, config->get_hash()}});
    if (!res.second)
    {
        // reset the configuration info without losing its previous view(s)
        res.first->second = {this->index.value(), config, pbft_config_state::accepted, res.first->second.value().views};
    }
}

pbft_configuration::shared_const_ptr
pbft_config_store::get(const hash_t& hash) const
{
    std::lock_guard<std::mutex> lock(this->lock);
    return this->private_get(hash);
}

pbft_configuration::shared_const_ptr
pbft_config_store::private_get(const hash_t& hash) const
{
    auto it = this->configs.find(hash);
    return it == this->configs.end() ? nullptr : it->second.value().config;
}

pbft_configuration::shared_const_ptr
pbft_config_store::get(uint64_t view) const
{
    std::lock_guard<std::mutex> lock(this->lock);
    auto it = this->view_configs.find(view);
    return it == this->view_configs.end() ? nullptr : this->private_get(it->second.value());
}

bool
pbft_config_store::set_prepared(const hash_t& hash)
{
    std::lock_guard<std::mutex> lock(this->lock);
    return this->set_state(hash, pbft_config_state::prepared);
}

bool
pbft_config_store::set_committed(const hash_t& hash)
{
    std::lock_guard<std::mutex> lock(this->lock);
    if (this->set_state(hash, pbft_config_state::committed))
    {
        // when a configuration is committed, we won't accept new_view with any earlier config
        for (auto& elem : this->configs)
        {
            auto it = this->configs.find(hash);
            assert(it != this->configs.end());

            if (elem.second.value().state != pbft_config_state::current && elem.second.value().index
                < it->second.value().index)
            {
                this->set_state(elem.first, pbft_config_state::deprecated);
            }
        }

        return true;
    }

    return false;
}

bool
pbft_config_store::set_current(const hash_t& hash, uint64_t view)
{
    std::lock_guard<std::mutex> lock(this->lock);
    if (this->view_configs.find(view) != this->view_configs.end())
    {
        LOG(error) << "Attempt to set configuration for a view that already has one: " << view;
        return false;
    }

    auto it = this->configs.find(hash);
    if (it != this->configs.end())
    {
        auto mutable_config{it->second.value()};
        mutable_config.state = pbft_config_state::current;
        mutable_config.views.insert(view);
        it->second = mutable_config;

        this->view_configs[view] = persistent<hash_t>{this->storage, hash, CONFIG_STORE_VIEW_CONFIGS_KEY, view};
        this->current_config = hash;
        return true;
    }

    return false;
}

pbft_configuration::shared_const_ptr
pbft_config_store::current() const
{
    std::lock_guard<std::mutex> lock(this->lock);
    auto it = this->configs.find(this->current_config.value());
    return it == this->configs.end() ? nullptr : it->second.value().config;
}

hash_t
pbft_config_store::newest_prepared() const
{
    std::lock_guard<std::mutex> lock(this->lock);
    return this->newest({pbft_config_state::prepared, pbft_config_state::committed, pbft_config_state::current});
}

hash_t
pbft_config_store::newest_committed() const
{
    std::lock_guard<std::mutex> lock(this->lock);
    return this->newest({pbft_config_state::committed, pbft_config_state::current});
}

pbft_config_store::pbft_config_state
pbft_config_store::get_state(const hash_t& hash) const
{
    auto it = this->configs.find(hash);
    return it == this->configs.end() ? pbft_config_state::unknown : it->second.value().state;
}

bool pbft_config_store::is_acceptable(const hash_t& hash) const
{
    std::lock_guard<std::mutex> lock(this->lock);
    return this->get_state(hash) != pbft_config_state::unknown && this->get_state(hash) != pbft_config_state::deprecated;
}

bool
pbft_config_store::set_state(const hash_t& hash, pbft_config_state state)
{
    auto it = this->configs.find(hash);
    if (it != this->configs.end())
    {
        auto mutable_config{it->second.value()};
        mutable_config.state = state;
        it->second = mutable_config;
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
        if (elem.second.value().index > max && std::any_of(states.begin(), states.end(), [elem](auto state)
        {
            return elem.second.value().state == state;
        }))
        {
            max = elem.second.value().index;
            hash = elem.first;
        }
    }

    return hash;
}

template <>
std::string
persistent<pbft_config_store::config_info>::to_string(const pbft_config_store::config_info& info)
{
    bzn::json_message json;
    json["index"] = info.index;
    json["config"] = info.config->to_json();
    json["state"] = static_cast<uint64_t>(info.state);

    json["views"] = bzn::json_message();
    for (auto view : info.views)
    {
        json["views"].append(view);
    }

    return json.toStyledString();
}

template <>
pbft_config_store::config_info
persistent<pbft_config_store::config_info>::from_string(const std::string& value)
{
    Json::Value json;
    Json::Reader reader;
    if (reader.parse(value, json))
    {
        if (json.isMember("index") && json.isMember("config") && json.isMember("state"))
        {
            try
            {
                pbft_config_store::config_info info;
                info.index = json["index"].asUInt64();

                pbft_configuration cfg;
                cfg.from_json(json["config"]);
                info.config = std::make_shared<pbft_configuration>(cfg);
                info.state = static_cast<pbft_config_store::pbft_config_state>(json["state"].asUInt64());

                for (auto& v : json["views"])
                {
                    info.views.insert(v.asUInt64());
                }

                return info;
            }
            catch(...)
            {}
        }
    }

    LOG(error) << "bad config_info from persistent state";
    throw std::runtime_error("bad config_info from persistent state");
}
