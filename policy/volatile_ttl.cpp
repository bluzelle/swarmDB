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

#include <policy/volatile_ttl.hpp>
#include <boost/src/boost/include/boost/lexical_cast.hpp>

namespace
{
    // TODO: This is a duplicate of code in crud, it would be a good idea to find a way to dry this up.
    const std::string TTL_UUID{"TTL"};
}


namespace bzn::policy
{
    struct ttl_item {std::string key; size_t value_size; size_t ttl;};

    std::set<bzn::key_t>
    volatile_ttl::keys_to_evict(const database_msg &request, size_t max_db_size)
    {
        std::set<bzn::key_t> keys_to_evict;

        auto db_uuid{request.header().db_uuid()};
        auto key_to_ignore{request.has_update() ? request.update().key() : "" };

        // get all the TTL's in storage
        auto all_ttls{this->storage->get_keys(TTL_UUID)};

        // get the TTL's associated with our database
        // filter out the ones for this database, if update, ignore current key
        auto our_ttls{this->filter_ttls(all_ttls, db_uuid, key_to_ignore)};
        if (our_ttls.empty())
        {
            return keys_to_evict;
        }

        // Create a vector of [key, value size, ttl]
        std::vector<ttl_item> ttl_items{get_ttl_items(db_uuid, our_ttls)};

        // 3: sort the keys in order of increasing ttl, secondarily sort on decreasing
        // key value size
        std::sort(
                ttl_items.begin(), ttl_items.end()
                , [](const auto& a, const auto& b)
                {
                    return a.value_size > b.value_size;
                });

        // 4: from the top of the list of keys pull out enough key value pairs
        //    to make room for the new pair or update,
        const auto current_db_size{this->storage->get_size(request.header().db_uuid()).second};
        const auto current_free = (max_db_size - current_db_size) - (request.has_update() ? request.update().value().size() : 0);
        const auto key_value_size {
            request.has_update()
            ? request.update().value().size()
            : request.create().key().size() + request.create().value().size()
        };

        // how much room do we need to free?
        //  enough for the new key value pair:  key_value_size
        // How much room do we have? -> current_free
        // How much do we need to free? -> required_size
        auto required_size = key_value_size - current_free;
        for (const auto ttl /*[key, size, ttl]*/ : ttl_items)
        {
            keys_to_evict.emplace(ttl.key);
            required_size -= (ttl.value_size < required_size ? ttl.value_size : required_size);
            if (!required_size)
            {
                break;
            }
        }

        // fail if we can't make enough room
        if (required_size)
        {
            keys_to_evict.clear();
        }

        // 5: return the list of keys to delete
        return keys_to_evict;
    }


    std::vector<ttl_item>
    volatile_ttl::get_ttl_items(const std::string &db_uuid, std::vector<std::string> &our_ttls) const
    {
        std::vector<ttl_item> ttl_items;
        auto char_reader{Json::CharReaderBuilder().newCharReader()};
        std::transform(our_ttls.begin(), our_ttls.end(), std::back_inserter(ttl_items),
                [&](const auto& ttl)->ttl_item
                {

                    Json::Value root;
                    std::string err;
                    if (char_reader->parse(ttl.c_str(), ttl.c_str() + ttl.size(), &root, &err))
                    {
                        const auto key{root["key"].asString()};
                        const auto expire_key{this->generate_expire_key(db_uuid, key)};
                        const auto opt_expire{storage->read(TTL_UUID, expire_key)};
                        const auto opt_value{storage->read(db_uuid, key)};

                        if (!opt_value.has_value())
                        {
                            LOG (warning) << "item with key:[" << key << "] does not exist in db: [" <<  db_uuid << "]";

                            return ttl_item{"", 0 , 0};
                        }

                        if (!opt_expire.has_value())
                        {
                            LOG (warning) << "TTL item with key:[" << expire_key << "] does not exist";

                            return ttl_item{"", 0, 0};
                        }

                        LOG (info) << "Found an item to evict: [" << key << "]" ;

                        return ttl_item{
                            key
                            , opt_value.value().size()
                            , boost::lexical_cast<uint64_t>(opt_expire.value())
                        };
                    }
                    LOG(warning) << "Unable to parse TTL for Volatile TTL eviction: " << err;

                    return ttl_item{"", 0 , 0};
                });

        return ttl_items;
    }


    std::vector<std::string>
    volatile_ttl::filter_ttls(const std::vector<std::string>& ttls, const bzn::uuid_t& db_uuid, const bzn::key_t& ignore_key)
    {
        std::vector<std::string> filtered_ttls;
        auto char_reader{Json::CharReaderBuilder().newCharReader()};
        std::copy_if(
                ttls.begin(), ttls.end(), std::back_inserter(filtered_ttls)
                , [&char_reader, &db_uuid, &ignore_key](const auto& t)
                {
                    Json::Value root;
                    std::string err;
                    if (char_reader->parse(t.c_str(), t.c_str() + t.size(), &root, &err))
                    {
                        bool s{(root["key"] != ignore_key) && (root["uuid"].asString() == db_uuid)};

                        return s;
                    }
                    LOG(warning) << "Unable to parse TTL for Volatile TTL eviction: " << err;

                    return false;
                });
        delete char_reader;

        return filtered_ttls;
    }
} // namespace bzn::crud::eviction
