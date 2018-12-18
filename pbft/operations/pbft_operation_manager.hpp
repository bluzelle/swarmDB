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
#include <pbft/operations/pbft_operation.hpp>
#include <include/bluzelle.hpp>
#include <storage/storage_base.hpp>
#include <optional>
#include <mutex>
#include <bootstrap/peer_address.hpp>

namespace bzn
{
    class pbft_operation_manager
    {
    public:
        pbft_operation_manager(std::optional<std::shared_ptr<bzn::storage_base>> storage = std::nullopt);

        /*
         * Returns a (possibly freshly constructed) pbft_operation for a particular view/sequence/request_hash.
         * Guarenteed to consistently return the same pbft_operation instance over the lifetime of the
         * pbft_operations_manager - this is important because pbft_operation only stores sessions in memory
         */
        std::shared_ptr<pbft_operation> find_or_construct(uint64_t view, uint64_t sequence, const bzn::hash_t& request_hash,
                                                          std::shared_ptr<const std::vector<bzn::peer_address_t>> peers_list);

        std::shared_ptr<pbft_operation> find_or_construct(const pbft_msg& msg, std::shared_ptr<const std::vector<bzn::peer_address_t>> peers_list);

        std::map<uint64_t, std::shared_ptr<pbft_operation>> prepared_operations_since(uint64_t sequence);

        size_t held_operations_count();

        void delete_operations_until(uint64_t sequence);

    private:
        std::mutex pbft_lock;
        const std::optional<std::shared_ptr<bzn::storage_base>> storage;

        std::map<bzn::operation_key_t, std::shared_ptr<pbft_operation>> held_operations;
    };
}
