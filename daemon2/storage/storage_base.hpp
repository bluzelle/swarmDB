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
#include <memory>
#include <string>
#include <chrono>

namespace bzn
{
    class storage_base
    {
    public:

        using record = struct
        {
            std::chrono::seconds timestamp;
            std::string          value;
            bzn::uuid_t          transaction_id;
        };

        virtual ~storage_base() = default;

        virtual void create(const bzn::uuid_t& uuid, const std::string& key, const std::string& value) = 0;

        virtual std::shared_ptr<bzn::storage_base::record> read(const bzn::uuid_t& uuid, const std::string& key) = 0;

        virtual void update(const bzn::uuid_t& uuid, const std::string& key, const std::string& value) = 0;

        virtual void remove(const bzn::uuid_t& uuid, const std::string& key) = 0;

        virtual void start() = 0;
    };

} // bzn
