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

#include <cstdint>
#include <string>


namespace bzn
{
    class ethereum_base
    {
    public:

        virtual ~ethereum_base() = default;

        /**
         * Fetch the balance associated with an ethereum address
         * @param address ethereum address
         * @param api_token ethereum api token
         * @return balance or zero if address is invalid
         */
        virtual double get_ether_balance(const std::string& address, const std::string& api_token) = 0;

    };

} // bzn
