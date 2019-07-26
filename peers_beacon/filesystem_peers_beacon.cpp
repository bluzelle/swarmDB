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

#include <peers_beacon/filesystem_peers_beacon.hpp>

using namespace blz;

filesystem_peers_beacon::filesystem_peers_beacon(std::shared_ptr<bzn::options_base> opt)
    : opt(std::move(opt))
{
}

void
filesystem_peers_beacon::start()
{
    this->refresh();
}

void
filesystem_peers_beacon::force_refresh()
{
    this->refresh();
}

void 
filesystem_peers_beacon::refresh()
{
    
}
