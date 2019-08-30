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

#include <peers_beacon/peer_address.hpp>
#include <peers_beacon/peers_beacon_base.hpp>
#include <node/node_base.hpp>
#include <pbft/operations/pbft_operation.hpp>
#include <proto/pbft.pb.h>
#include <optional>
#include <string>


namespace bzn
{
    using client_t = std::string; //placeholder

    class pbft_base
    {
    public:
        virtual void start() = 0;

        virtual void handle_message(const pbft_msg& msg, const bzn_envelope& original_msg) = 0;

        virtual void handle_database_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> session) = 0;

        virtual bool is_primary() const = 0;

        virtual peer_address_t get_primary(std::optional<uint64_t> view) const = 0;

        virtual const bzn::uuid_t& get_uuid() const = 0;

        virtual void handle_failure() = 0;

        virtual const peer_address_t& get_peer_by_uuid(const std::string& uuid) const = 0;

        virtual ~pbft_base() = default;

    };
}
