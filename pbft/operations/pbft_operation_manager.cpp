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

#include <pbft/operations/pbft_operation_manager.hpp>
#include <pbft/operations/pbft_memory_operation.hpp>
#include <pbft/operations/pbft_persistent_operation.hpp>
#include <utils/bytes_to_debug_string.hpp>
#include <boost/format.hpp>

using namespace bzn;

pbft_operation_manager::pbft_operation_manager(std::shared_ptr<bzn::peers_beacon_base> peers, std::optional<std::shared_ptr<bzn::storage_base>> storage)
    : peers(peers)
    , storage(storage)
{
    if (!storage)
    {
        LOG(warning) << "pbft operation operation manager constructed without a storage backend; operations will not be persistent";
    }
}

std::shared_ptr<pbft_operation>
pbft_operation_manager::find_or_construct(uint64_t view, uint64_t sequence, const bzn::hash_t &request_hash)
{
    std::lock_guard<std::mutex> lock(this->pbft_lock);

    auto key = bzn::operation_key_t(view, sequence, request_hash);

    auto lookup = this->held_operations.find(key);
    if (lookup == this->held_operations.end())
    {
        LOG(debug) << "Creating operation for seq " << sequence << " view " << view << " req " << bytes_to_debug_string(request_hash);

        std::shared_ptr<pbft_operation> op;
        if (this->storage)
        {
            op = std::make_shared<pbft_persistent_operation>(view, sequence, request_hash, *(this->storage), this->peers);
        }
        else
        {
            op = std::make_shared<pbft_memory_operation>(view, sequence, request_hash, this->peers);
        }

        bool added;
        std::tie(std::ignore, added) = this->held_operations.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple(op));
        assert(added);

        return op;
    }

    return lookup->second;
}

std::shared_ptr<pbft_operation>
pbft_operation_manager::find_or_construct(const pbft_msg& msg)
{
    return this->find_or_construct(msg.view(), msg.sequence(), msg.request_hash());
}

void
pbft_operation_manager::delete_operations_until(uint64_t sequence)
{
    std::lock_guard<std::mutex> lock(this->pbft_lock);

    size_t ops_removed = 0;
    auto it = this->held_operations.begin();
    while (it != this->held_operations.end())
    {
        if (it->second->get_sequence() <= sequence)
        {
            it = this->held_operations.erase(it);
            ops_removed++;
        }
        else
        {
            it++;
        }
    }

    LOG(debug) << boost::format("Cleared %1% old operation records") % ops_removed;

    if (this->storage)
    {
        LOG(debug) << "cleaning up operation state from storage";
        pbft_persistent_operation::remove_range(*this->storage, 0, sequence);
    }
}

std::map<uint64_t, std::shared_ptr<pbft_operation>>
pbft_operation_manager::prepared_operations_since(uint64_t sequence)
{
    // If there are multiple operations for a sequence number, we may return arbitrarily one of them, so long as we
    // choose one thats prepared. We choose the one in the most recent view, which maximizes the likelihood of some
    // simpleness with respect to dynamic peering. There cannot be multiple prepared operations with distinct
    // request hashes because we wouldn't accept the preprepares.

    std::map<uint64_t, std::shared_ptr<pbft_operation>> result;
    const auto maybe_store = [&](const std::shared_ptr<pbft_operation>& op)
    {
        const auto search = result.find(op->get_sequence());
        if (search == result.end() || result[op->get_sequence()]->get_view() < op->get_view())
        {
            result[op->get_sequence()] = op;
        }
    };

    if (this->storage)
    {
        for (const auto& op : pbft_persistent_operation::prepared_operations_in_range(*this->storage, this->peers, sequence + 1))
        {
            maybe_store(op);
        }
    }
    else
    {
        for (const auto& pair : this->held_operations)
        {
            if (pair.second->get_sequence() > sequence && pair.second->is_prepared())
            {
                maybe_store(pair.second);
            }
        }
    }

    return result;
}

size_t
pbft_operation_manager::held_operations_count()
{
    return this->held_operations.size();
}
