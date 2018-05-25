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

#include <node/session_base.hpp>
#include <gmock/gmock.h>


// gmock_gen.py generated...

namespace bzn {
    class Mocksession_base : public session_base {
    public:
        MOCK_METHOD1(start,
                     void(bzn::message_handler handler));
        MOCK_METHOD2(send_message,
                     void(std::shared_ptr<const bzn::message> msg, bool end_session));
        MOCK_METHOD0(close,
                    void());
    };
}  // namespace bzn
