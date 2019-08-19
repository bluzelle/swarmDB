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

#include <peers_beacon/peers_beacon.hpp>

namespace bzn
{
    class filesystem_peers_beacon : public peers_beacon
    {
    public:
        filesystem_peers_beacon(std::shared_ptr<bzn::options_base> opt);

        bool force_refresh() override;

    };
}
