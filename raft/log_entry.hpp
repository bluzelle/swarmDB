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

#include <ostream>
#include <boost/beast/core/detail/base64.hpp>
#include <string>


namespace bzn
{
    const std::string MSG_ERROR_ENCOUNTERED_INVALID_ENTRY_IN_LOG{"ENCOUNTERED_INVALID_ENTRY_IN_LOG"};
    const std::string MSG_ERROR_ENCOUNTERED_INVALID_ENTRY_TYPE_IN_LOG{"ENCOUNTERED_INVALID_ENTRY_TYPE_IN_LOG"};

    enum class log_entry_type : uint8_t
    {
        log_entry,
        single_quorum,
        joint_quorum,
        undefined
    };


    struct log_entry
    {
        friend std::ostream &operator<<(std::ostream& out, const log_entry& obj)
        {
            out << static_cast<uint8_t >(obj.entry_type) << " " << obj.log_index << " " << obj.term << " " << boost::beast::detail::base64_encode(obj.json_to_string(obj.msg)) << "\n";
            return out;
        }


        friend std::istream &operator>>(std::istream& in, log_entry& obj)
        {
            std::string msg_string;
            obj.log_index = std::numeric_limits<uint32_t>::max();
            obj.term = std::numeric_limits<uint32_t>::max();

            auto entry_type = static_cast<uint8_t>(bzn::log_entry_type::undefined);
            in >> entry_type >> obj.log_index >> obj.term >> msg_string;
            obj.entry_type = static_cast<log_entry_type >(entry_type);
            
            if (!msg_string.empty())
            {
                Json::Reader reader;
                if (!reader.parse(boost::beast::detail::base64_decode(msg_string), obj.msg))
                {
                    throw std::runtime_error(bzn::MSG_ERROR_ENCOUNTERED_INVALID_ENTRY_IN_LOG + ":" + reader.getFormattedErrorMessages());
                }
            }
            return in;
        }


        inline std::string
        json_to_string(const bzn::message& msg) const
        {
            Json::FastWriter fastWriter;
            return fastWriter.write(msg);
        }

        log_entry_type  entry_type;
        uint32_t        log_index;
        uint32_t        term;
        bzn::message    msg;
    };
}

