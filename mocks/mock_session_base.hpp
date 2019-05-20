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
    class mock_session_base : public session_base {
    public:
        MOCK_METHOD1(send_message,
            void(std::shared_ptr<bzn::encoded_message> msg));
        MOCK_METHOD1(send_signed_message,
                void(std::shared_ptr<bzn_envelope> msg));
        MOCK_METHOD0(get_session_id,
            bzn::session_id());
        MOCK_METHOD0(close,
            void());
        MOCK_CONST_METHOD0(is_open,
            bool());
        MOCK_METHOD1(open,
            void(std::shared_ptr<bzn::beast::websocket_base> ws_factory));
        MOCK_METHOD1(accept,
            void(std::shared_ptr<bzn::beast::websocket_stream_base> ws));
        MOCK_METHOD1(add_shutdown_handler,
            void(bzn::session_shutdown_handler handler));

    };
}  // namespace bzn
