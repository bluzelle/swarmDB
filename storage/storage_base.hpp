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
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>

namespace bzn
{
    const size_t MAX_VALUE_SIZE = 307200;

    class storage_base
    {
    public:

        struct record
        {
            friend class boost::serialization::access;
            std::chrono::seconds timestamp;
            std::string          value;
            bzn::uuid_t          transaction_id;

            template <class Archive>
            void
            serialize(Archive& ar, const unsigned int /*version*/)
            {
                auto ts = this->timestamp.count();

                ar & this->value & this->transaction_id & ts;

                this->timestamp = std::chrono::seconds(ts);
            }
        };

        enum class result : uint8_t
        { ok=0, not_found, exists, not_saved, value_too_large };

        virtual ~storage_base() = default;

        virtual storage_base::result create(const bzn::uuid_t& uuid, const std::string& key, const std::string& value) = 0;

        virtual std::shared_ptr<bzn::storage_base::record> read(const bzn::uuid_t& uuid, const std::string& key) = 0;

        virtual storage_base::result update(const bzn::uuid_t& uuid, const std::string& key, const std::string& value) = 0;

        virtual storage_base::result remove(const bzn::uuid_t& uuid, const std::string& key) = 0;

        virtual storage_base::result save(const std::string& path) = 0;

        virtual storage_base::result load(const std::string& path) = 0;

        virtual std::vector<std::string> get_keys(const bzn::uuid_t& uuid) = 0;
        
        virtual bool has(const bzn::uuid_t& uuid, const  std::string& key) = 0;

        virtual std::size_t get_size(const bzn::uuid_t& uuid) = 0;
    };
} // bzn
