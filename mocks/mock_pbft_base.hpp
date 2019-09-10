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

#include <pbft/pbft_base.hpp>
#include <gmock/gmock.h>


// gmock_gen.py generated...

namespace bzn {

class mock_pbft_base : public pbft_base {
public:
    MOCK_METHOD0(start,
            void());
    MOCK_METHOD2(handle_message,
            void(const pbft_msg& msg, const bzn_envelope& original_msg));
    MOCK_METHOD2(handle_database_message,
            void(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session));
    MOCK_CONST_METHOD0(is_primary,
            bool());
    MOCK_CONST_METHOD1(get_primary,
            peer_address_t(std::optional<uint64_t> view));
    MOCK_CONST_METHOD0(get_uuid,
            const bzn::uuid_t&());
    MOCK_METHOD0(handle_failure,
            void());
    MOCK_CONST_METHOD1(get_peer_by_uuid,
            const peer_address_t&(const std::string& uuid));
    MOCK_CONST_METHOD0(peers,
        std::shared_ptr<bzn::peers_beacon_base>());
};

}  // namespace bzn
