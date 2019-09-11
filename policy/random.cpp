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

#include <policy/random.hpp>
#include <proto/database.pb.h>
#include <boost/src/boost/include/boost/random/mersenne_twister.hpp>
#include <boost/src/boost/include/boost/random/uniform_int_distribution.hpp>

namespace bzn::policy
{
    std::set<std::string>
    random::keys_to_evict(const database_msg &request, size_t max_size)
    {
        const auto KEY_VALUE_SIZE{
            request.has_update()
            ? request.update().value().size()
            : request.create().key().size() + request.create().value().size()
        };

        const bzn::key_t IGNORE_KEY{
            request.has_update()
            ? request.update().key()
            : ""
        };

        std::vector<bzn::key_t> keys_to_evict;
        const auto size{this->storage->get_size(request.header().db_uuid()).second};
        size_t storage_to_free{KEY_VALUE_SIZE - (max_size - size)};

        // We may need to remove one or more key/value pairs to make room for the new one
        std::hash<std::string> hasher;
        boost::random::mt19937 mt(hasher(request.header().request_hash()));

        auto available_keys = this->storage->get_keys(request.header().db_uuid());
        while (storage_to_free && !available_keys.empty())
        {
            // As we randomly select a key from the keys in the user's database, we *move* them from the vector of keys
            // in the users' database to the vector of keys that need to be deleted. In the case of randomly selected
            // the key that is being updated, we simnply remove that key from the vector of users' keys without putting
            // it in the keys to delete.
            const boost::random::uniform_int_distribution<> dist(0, available_keys.size() - 1);
            const auto it = std::next(available_keys.begin(), dist(mt));
            if (*it != IGNORE_KEY)
            {
                if (const auto evicted_size = this->storage->get_key_size(request.header().db_uuid(), *it))
                {
                    std::move(it, std::next(it,1), std::back_inserter(keys_to_evict));
                    storage_to_free -= *evicted_size < storage_to_free ? *evicted_size : storage_to_free;
                }
            }
            available_keys.erase(it, std::next(it,1));
        }

        // Did we free enough storage?
        if (!storage_to_free)
        {
            // It would have been nice to move the keys into the set directly, but std::move doesn't move from vector to set

            return std::set<bzn::key_t>(keys_to_evict.begin(), keys_to_evict.end());
        }

        return std::set<bzn::key_t>{};
    }
} // namespace bzn::crud::eviction
