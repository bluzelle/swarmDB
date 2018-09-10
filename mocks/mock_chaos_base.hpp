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

#include <functional>
#include <gmock/gmock.h>

namespace bzn {

class mock_chaos_base : public chaos_base {
 public:
  MOCK_METHOD0(start,
      void());
  MOCK_METHOD0(is_message_dropped,
      bool());
  MOCK_METHOD0(is_message_delayed,
      bool());
  MOCK_CONST_METHOD1(reschedule_message,
      void(std::function<void()> callback));
};

}  // namespace bzn
