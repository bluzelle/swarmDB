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

#include <crud/subscription_manager_base.hpp>
#include <gmock/gmock.h>

namespace bzn {

class Mocksubscription_manager_base : public subscription_manager_base {
 public:
  MOCK_METHOD0(start,
      void());
  MOCK_METHOD5(subscribe,
      void(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session));
  MOCK_METHOD5(unsubscribe,
      void(const bzn::uuid_t& uuid, const bzn::key_t& key, uint64_t transaction_id, database_response& response, std::shared_ptr<bzn::session_base> session));
  MOCK_METHOD1(inspect_commit,
      void(const database_msg& msg));
};

}  // namespace bzn
