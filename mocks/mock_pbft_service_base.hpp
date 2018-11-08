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
#include <gmock/gmock.h>
#include <functional>

namespace bzn {

    class mock_pbft_service_base : public pbft_service_base {
     public:
      MOCK_METHOD1(apply_operation,
          void(std::shared_ptr<bzn::pbft_operation>));
      MOCK_CONST_METHOD2(query,
          void(const pbft_request& request, uint64_t sequence_number));
      MOCK_CONST_METHOD1(service_state_hash,
          bzn::hash_t(uint64_t sequence_number));
      MOCK_METHOD1(consolidate_log,
          void(uint64_t sequence_number));
      MOCK_METHOD1(register_execute_handler,
          void(bzn::execute_handler_t handler));
      MOCK_METHOD1(apply_operation,
          void(const std::shared_ptr<pbft_operation>&));
      MOCK_CONST_METHOD1(get_service_state,
          bzn::service_state_t(uint64_t sequence_number));
      MOCK_METHOD2(set_service_state,
          bool(uint64_t sequence_number, const bzn::service_state_t& data));
    };

}  // namespace bzn
