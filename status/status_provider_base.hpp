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


namespace bzn
{
    class status_provider_base
    {
    public:
        virtual ~status_provider_base() = default;

        /**
         * get name
         * @return name
         */
        virtual std::string get_name() = 0;

        /**
         * get status
         * @return status in a json message
         */
        virtual bzn::json_message get_status() = 0;
    };

} // namespace bzn
