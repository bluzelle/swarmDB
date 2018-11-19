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

#include <crud/crud_base.hpp>
#include <gmock/gmock.h>

namespace bzn {

    namespace deprecated
    {
        class Mockcrud_base : public deprecated::crud_base {
        public:
            MOCK_METHOD3(handle_create,
                void(const bzn::json_message& msg, const database_msg& request, database_response& response));
            MOCK_METHOD3(handle_read,
                void(const bzn::json_message& msg, const database_msg& request, database_response& response));
            MOCK_METHOD3(handle_update,
                void(const bzn::json_message& msg, const database_msg& request, database_response& response));
            MOCK_METHOD3(handle_delete,
                void(const bzn::json_message& msg, const database_msg& request, database_response& response));
            MOCK_METHOD0(start,
                void());
        };
    }

    class Mockcrud_base : public crud_base {
    public:
        MOCK_METHOD2(handle_request,
            void(const database_msg& request, const std::shared_ptr<bzn::session_base>& session));
        MOCK_METHOD0(start,
            void());
        MOCK_METHOD0(save_state,
            bool());
        MOCK_METHOD0(get_saved_state,
            std::shared_ptr<std::string>());
        MOCK_METHOD1(load_state,
            bool(const std::string&));
    };

}  // namespace bzn

