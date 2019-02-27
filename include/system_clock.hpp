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
#include <chrono>

namespace bzn
{
    class system_clock_base
    {
    public:
        virtual uint64_t microseconds_since_epoch() = 0;

        virtual ~system_clock_base() = default;
    };

    class system_clock : public system_clock_base
    {
        uint64_t microseconds_since_epoch()
        {
            auto res = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now());
            return res.time_since_epoch().count();
        }
    };
}
