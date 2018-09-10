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
#include <functional>

namespace bzn
{
    class chaos_base
    {
    public:
        /*
         * start the node death timer (if enabled)
         */
        virtual void start() = 0;

        /*
         * Randomly determine if a message should be dropped, according to settings
         */
        virtual bool is_message_dropped() = 0;

        /*
         * Randomly determine if a message should be delayed, according to settings
         */
        virtual bool is_message_delayed() = 0;

        /*
         * Schedule a delayed callback according to settings
         * @param callback      callback for sending message when delay expires
         */
        virtual void reschedule_message(std::function<void()> callback) const = 0;

        virtual ~chaos_base() = default;
    };
}
