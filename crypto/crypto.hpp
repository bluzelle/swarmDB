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
//

#pragma once

#include <crypto/crypto_base.hpp>
#include <options/options_base.hpp>
#include <proto/bluzelle.pb.h>
#include <openssl/evp.h>
#include <openssl/ec.h>

namespace bzn
{
    class crypto : public crypto_base
    {
    public:

        crypto(std::shared_ptr<bzn::options_base> options);

        void start();

        bool sign(wrapped_bzn_msg& msg) override;

        bool verify(const wrapped_bzn_msg& msg) override;

        ~crypto() override;

    private:

        bool load_private_key();

        void log_openssl_errors();

        std::shared_ptr<bzn::options_base> options;

        EVP_PKEY* private_key_EVP = NULL;
        EC_KEY* private_key_EC = NULL;

    };
}

