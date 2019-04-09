// Copyright (C) 2019 Bluzelle
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

#include <mocks/mock_node_base.hpp>
#include <unordered_map>

namespace bzn
{
    class smart_mock_node : public Mocknode_base
    {
    public:
        smart_mock_node();

        void deliver(const bzn_envelope&);

        std::unordered_map<bzn_envelope::PayloadCase, bzn::protobuf_handler> registrants;

    };
}
