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

#include <bootstrap/peer_address.hpp>
#include <bootstrap/bootstrap_peers_base.hpp>


namespace bzn
{

    class bootstrap_peers final : public bzn::bootstrap_peers_base
    {
    public:
        bool fetch_peers_from_file(const std::string& filename) override;

        bool fetch_peers_from_url(const std::string& url) override;

        const bzn::peers_list_t& get_peers() const override;

    private:
        bool ingest_json(std::istream& peers);

        bzn::peers_list_t peer_addresses;

        size_t initialize_peer_list(const Json::Value& root, bzn::peers_list_t& peer_addresses);
    };

} // namespace bzn
