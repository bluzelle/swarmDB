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

#include <peers_beacon/peers_beacon_base.hpp>
#include <gmock/gmock.h>


namespace bzn {

    class mock_peers_beacon_base : public peers_beacon_base
    {
    public:
        MOCK_METHOD0(start,
            void());

        MOCK_CONST_METHOD0(current,
            std::shared_ptr<const peers_list_t>());

        MOCK_CONST_METHOD0(ordered,
                std::shared_ptr<const ordered_peers_list_t>());

        MOCK_METHOD1(refresh,
            bool(bool first_run));

        bool refresh()
        {
            return this->refresh(false);
        }
    };

}
