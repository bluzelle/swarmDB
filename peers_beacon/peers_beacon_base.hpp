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

namespace bzn
{
    class peers_beacon_base
    {
        virtual void start() = 0;

        // get a pointer to the current peers list (which won't change, but can be replaced at any time)
        virtual std::shared_ptr<const peers_list_t> current() const = 0;

        // refresh from whatever source we are using; return success
        virtual bool refresh(bool first_run = false) = 0;

        virtual ~peers_beacon();
    };
}
