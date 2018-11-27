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

#include <include/boost_asio_beast.hpp>
#include <crud/crud_base.hpp>
#include <pbft/pbft_failure_detector_base.hpp>
#include <pbft/pbft_service_base.hpp>
#include <storage/storage_base.hpp>
#include <memory>


namespace bzn
{
    class database_pbft_service final : public bzn::pbft_service_base, public std::enable_shared_from_this<database_pbft_service>
    {
    public:
        database_pbft_service(std::shared_ptr<bzn::asio::io_context_base> io_context,
                              std::shared_ptr<bzn::storage_base> unstable_storage,
                              std::shared_ptr<bzn::crud_base> crud,
                              bzn::uuid_t uuid);

        virtual ~database_pbft_service();

        void apply_operation(const std::shared_ptr<bzn::pbft_operation>& op) override;

        bzn::hash_t service_state_hash(uint64_t sequence_number) const override;

        std::shared_ptr<bzn::service_state_t> get_service_state(uint64_t sequence_number) const override;

        bool set_service_state(uint64_t sequence_number, const bzn::service_state_t& data) override;

        void save_service_state_at(uint64_t sequence_number) override;

        void consolidate_log(uint64_t sequence_number) override;

        void register_execute_handler(bzn::execute_handler_t handler) override;

        uint64_t applied_requests_count() const;

    private:
        void process_awaiting_operations();

        void load_next_request_sequence();
        void save_next_request_sequence();

        std::shared_ptr<bzn::asio::io_context_base> io_context;
        std::shared_ptr<bzn::storage_base> unstable_storage;
        std::shared_ptr<bzn::crud_base> crud;
        uint64_t next_request_sequence = 1;
        const bzn::uuid_t uuid;

        std::unordered_map<uint64_t, std::shared_ptr<bzn::pbft_operation>> operations_awaiting_result;

        bzn::execute_handler_t execute_handler;

        std::once_flag start_once;
        std::mutex lock;
        uint64_t next_checkpoint = 0;
        uint64_t last_checkpoint = 0;
    };

} // bzn
