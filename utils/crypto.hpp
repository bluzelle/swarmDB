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

#include <string>

namespace bzn::utils::crypto
{
    /**
     * base64_decode is used by verify_signature to decode the "human readable"
     * signature text into it's original binary format. We could not use the
     * base64 decoder in boost::beast::core::detail as those functions take stl
     * strings as input and output, so binary output with embedded '\0' are not
     * handled correctly.
     *
     * @param base64_message pointer to a character buffer containing the base64 encoded string
     * @param in/out vector of unsigned chars that will contain the decoded data
     * @return integer value of 0 on success
     */
    int base64_decode(const std::string& base64_message, std::vector<unsigned char>& decoded_message);


    /**
     * Retrieves the Bluzelle public key from the Etherium blockchain via a call
     * to a contract
     * @return a string containing the base64 Encoded Bluzelle public key
     */
    std::string
    retrieve_bluzelle_public_key_from_contract();


    /**
     * verify_signature is a C++ wrapper for the C style OpenSSL signature
     * verification code. It provides an interface that uses STL strings
     * instead of raw pointers to buffers.
     *
     * @param public_key the PEM formatted public key provided by the Bluzelle rep
     * @param signature the base 64 encocoded signature file created from the node uuid and the Bluzelle private key
     * @param uuid the uuid of the node that is to be added to the swarm
     * @return true if the signature is valid, false otherwise.
     * @throws runtime_error if the public key is invalid, the signature could not be decoded or the validation
     * failed before finishing due to some error.
     */
    bool verify_signature(const std::string& public_key, const std::string& signature, const std::string& uuid);
}
