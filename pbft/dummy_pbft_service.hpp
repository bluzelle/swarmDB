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

#include <pbft/pbft_service_base.hpp>
#include <pbft/pbft_failure_detector_base.hpp>
#include <include/boost_asio_beast.hpp>
#include <unordered_map>

namespace bzn
{
    // This is a placeholder implementation! Eventually, a pbft_service will
    // wrap crud. It needs to be that way around (rather than crud calling
    // pbft) because pbft needs to decide when crud gets to see messages.

    class dummy_pbft_service : public pbft_service_base
    {
    public:
        dummy_pbft_service(std::shared_ptr<bzn::asio::io_context_base> io_context);
        void apply_operation(const pbft_request& request, uint64_t sequence_number) override;
        void query(const pbft_request& request, uint64_t sequence_number) const override;
        void consolidate_log(uint64_t sequence_number) override;
        void register_execute_handler(execute_handler_t handler) override;
        bzn::hash_t service_state_hash(uint64_t sequence_number) const override;

        uint64_t applied_requests_count();

    private:
        execute_handler_t execute_handler;
        std::shared_ptr<bzn::asio::io_context_base> io_context;

        uint64_t next_request_sequence = 1;
        std::unordered_map<uint64_t, pbft_request> waiting_requests;
        std::mutex lock;
    };

}
