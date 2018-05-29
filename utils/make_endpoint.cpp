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
#include "make_endpoint.hpp"


boost::asio::ip::tcp::endpoint
bzn::make_endpoint(const bzn::peer_address_t& peer)
{
    return boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(peer.host), peer.port);
}
