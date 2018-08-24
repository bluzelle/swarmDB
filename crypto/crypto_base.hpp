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

#include <proto/bluzelle.pb.h>

namespace bzn
{
    class crypto_base
    {
    public:
        /*
         * sign a message using our key
         * @msg message to sign
         * @return if signature was successful
         */
        virtual bool sign(wrapped_bzn_msg& msg) = 0;

        /*
         * verify that the signature on a message is correct and matches its sender
         * @msg message to verify
         * @return signature is present, valid and matches sender
         */
        virtual bool verify(const wrapped_bzn_msg& msg) = 0;

        virtual ~crypto_base() = default;
    };
}


