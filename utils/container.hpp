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

#include <include/bluzelle.hpp>
#include <bootstrap/bootstrap_peers_base.hpp>
#include <random>

namespace
{
    std::mt19937 gen(std::time(0)); //Standard mersenne_twister_engine seeded with rd()
}


namespace bzn::utils::container
{
    // TODO: RHN - this should be templatized
    bzn::peers_list_t::const_iterator
    choose_any_one_of(const bzn::peers_list_t& all_peers)
    {
        if (all_peers.size()>0)
        {
            std::uniform_int_distribution<> dis(1, all_peers.size());
            auto it = all_peers.begin();
            std::advance(it, dis(gen) - 1);
            return it;
        }
        return all_peers.end();
    }
}
