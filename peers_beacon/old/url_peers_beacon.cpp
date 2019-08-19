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

#include <peers_beacon/url_peers_beacon.hpp>

using namespace blz;

url_peers_beacon::url_peers_beacon(std::shared_ptr<bzn::options_base> opt)
        : peers_beacon(std::move(opt))
{
}

bool
url_peers_beacon::force_refresh()
{

}

