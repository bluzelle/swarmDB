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

#include <utils/bytes_to_debug_string.hpp>
#include <include/bluzelle.hpp>

#include <iostream>
#include <sstream>

std::string
bzn::bytes_to_debug_string(const std::string& input, bool preserve_full)
{
    std::stringstream ss;
    ss << std::hex;

    for(const char& c : input)
    {
        // This is not a remotely rigorous way to do this, but as long as the goal is just to make
        // hashes distinguishable in logs it will suffice.
        ss << std::hex << (0x00ff & static_cast<const unsigned short&>(c));
    }

    auto result = ss.str();
    if (result.size() > MAX_SHORT_MESSAGE_SIZE && !preserve_full)
    {
        result.erase(MAX_SHORT_MESSAGE_SIZE/2, result.size() - MAX_SHORT_MESSAGE_SIZE);
        result.insert(MAX_SHORT_MESSAGE_SIZE/2, "...");
    }

    return result;
}