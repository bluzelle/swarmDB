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

/**
 * The uuid/public key/signature validation code for this module was taken
 * with modification from
 *
 * https://eclipsesource.com/blogs/2016/09/07/tutorial-code-signing-and-verification-with-openssl/
 *
 * This module duplicates the following functionality that can be performed
 * on the command line: convert base64 encoded signature to binary
 *
 *          openssl base64 -d -in signature.txt -out /tmp/sign.sha256
 * verify the uuid
 *          openssl dgst -sha256 -verify public.pem -signature /tmp/sign.sha256 uuid.txt
 */

#include <include/bluzelle.hpp>
#include "crypto.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <cstdio>
#include <curl/curl.h>
#include <boost/format.hpp>
#include <iomanip>

namespace
{
    const std::string ROPSTEN_URL{"https://ropsten.infura.io"};
    const std::string REQUEST_BASE              {R"({"jsonrpc":"2.0","method":"eth_call","params":[{"to":"0x492CCD1eAeCDA0262e3AAeF8445F3F731a30AfbB","data": "%s" },"latest"],"id":1})"};
    const std::string GET_KEY_SIZE              {"0xcf5d53f1"};
    const std::string GET_KEY_CHUNK             {"0x96ce93e2"};
    const std::string MSG_ERROR_CURL            {"curl_easy_perform() failed: "};

    /**
     * This function is used by base_64_decode to determine the amount of memory required
     * for the binary buffer represented by the base64 encoded string.
     *
     * @param base_64_input base64 encoded string
     * @return a size_t containing the number of bytes expected in the decoded string.
    */
    size_t
    calculate_decode_length(const std::string& base_64_input)
    {
        if (base_64_input.empty())
        {
            return 0;
        }
        const size_t len{base_64_input.size()};
        const size_t padding = (base_64_input[len - 1] == '=' && base_64_input[len - 2] == '=') ? 2 : 1;
        return (len * 3) / 4 - padding;
    }


    void
    compose_and_log_OpenSSL_error(const std::string& failing_function_name)
    {
        char buffer[126]{0};
        ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
        LOG(error) << "While verifiying a node uuid " << failing_function_name << " failed with error " << buffer;
    }


    /**
     * Given the public key string in PEM format, this function will return the
     * RSA public key structure that OpenSSL functions use.
     *
     * @param key a const std::string reference containing the public key in PEM format
     * @return a pointer to the RSA structure contqining the public key
     */
    RSA* create_public_RSA(const std::string& key)
    {
        std::unique_ptr<BIO,std::function<void(BIO*)>> keybio(BIO_new_mem_buf((void*)(key.c_str()), -1), BIO_free);
        if (!keybio)
        {
            return 0;
        }

        // Note that calling RSA_free on rsa will cause a segfault at it is not allocated in the PEM function. So I am
        // not going to create a unique_ptr in this case.
        RSA* rsa{nullptr};
        PEM_read_bio_RSA_PUBKEY(keybio.get(), &rsa, nullptr, nullptr);
        return rsa;
    }


    /**
     * Given the RSA structure containing the Bluzelle public key, the
     * signature of the node's uuid, and the node's uuid, this method verifies
     * that the signature is valid.
     *
     * @param rsa a pointer to the public key structure
     * @param signature a const vector reference to the signature associated with the uuid
     * @param uuid a const string reference to the nodes' uuid
     * @param is_authentic an reference to a bool that will be set to true if the signature is valid, false otherwise.
     * @return a bool that returns true if the validation process was successful.
     */
    bool
    RSA_verify_signature(const RSA* rsa, const std::vector<unsigned char>& signature, const std::string& uuid, bool& is_authentic)
    {
        std::unique_ptr<EVP_PKEY, std::function<void(EVP_PKEY*)>> public_key{EVP_PKEY_new(), EVP_PKEY_free};

        std::unique_ptr<EVP_MD_CTX, std::function<void(EVP_MD_CTX*)>> RSA_verification_context{EVP_MD_CTX_new(), EVP_MD_CTX_free};

        // I could find no guidance in the OpenSSL documentation that suggests that the EVP_MD
        // pointer needs to be released after use, though in the same page the documentation
        // points out that EVP_MD_CTX pointers *do* need to be released. For now I will leave
        // the following as a raw pointer.
        const EVP_MD* sha256_message_digest = EVP_sha256();

        is_authentic = false;
        EVP_PKEY_assign_RSA(public_key.get(), rsa);

        EVP_DigestInit_ex( RSA_verification_context.get(), sha256_message_digest, nullptr);

        if (1 != EVP_DigestVerifyInit(RSA_verification_context.get(), nullptr, sha256_message_digest, nullptr, public_key.get()))
        {
            compose_and_log_OpenSSL_error("EVP_DigestVerifyInit");
            return false;
        }

        if (1 != EVP_DigestVerifyUpdate(RSA_verification_context.get(), uuid.c_str(), uuid.size()))
        {
            compose_and_log_OpenSSL_error("EVP_DigestVerifyUpdate");
            return false;
        }
        // The const cast is for the unix gcc c++ compiler
        const int auth_status = EVP_DigestVerifyFinal(RSA_verification_context.get(), const_cast<unsigned char*>(signature.data()), signature.size());

        // There are a couple of options here, the signature was successfully
        // validated or invalidated, or something went wrong during the
        // validation process.
        if ( auth_status==0 || auth_status==1)
        {
            is_authentic = (auth_status==1);
            return true;
        }
        else
        {
            compose_and_log_OpenSSL_error("EVP_DigestVerifyUpdate");
            return false;
        }
    }


    size_t
    write_function(void* contents,size_t size, size_t buffer_size, void* userp)
    {
        reinterpret_cast<std::string*>(userp)->append(reinterpret_cast<char*>(contents), size * buffer_size);
        return size * buffer_size;
    }


    bzn::json_message
    get_curl_response(const std::string& function_call, const std::string& url = ROPSTEN_URL)
    {
        std::unique_ptr<CURL, std::function<void(CURL*)>> curl(curl_easy_init(),
                                                               std::bind(curl_easy_cleanup, std::placeholders::_1));
        std::string readBuffer;

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);

        const std::string post_fields{boost::str(boost::format(REQUEST_BASE) % function_call)};

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, post_fields.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, -1L);
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, reinterpret_cast<void*>(write_function));

        Json::Reader reader;

        CURLcode res = curl_easy_perform(curl.get());
        if (res != CURLE_OK)
        {
            LOG(error) << MSG_ERROR_CURL << " - " << curl_easy_strerror(res);
        }

        bzn::json_message response;

        if (!reader.parse(readBuffer, response))
        {
            LOG(error) << "Unable to parse response from Ropsten - could not get key size";
        }
        return response;
    }


    // curl --data '{"jsonrpc":"2.0","method":"eth_call","params":[{"to":"0x492CCD1eAeCDA0262e3AAeF8445F3F731a30AfbB","data": "0xcf5d53f1" },"latest"],"id":1}' https://ropsten.infura.io
    size_t
    get_public_pem_size(const std::string& url = ROPSTEN_URL)
    {
        try
        {
            const auto response = get_curl_response(GET_KEY_SIZE, url);
            // result will look like "0x0000000000000000000000000000000000000000000000000000000000000012"
            return std::stoul(response["result"].asString().c_str(), nullptr, 16) ;
        }
        catch(std::exception& ex)
        {
            LOG(error) << "Invalid PEM size response from Bluzelle public key request: " << ex.what();
        }
        return 0;
    }



    std::string
    integer_to_32_byte_hex(const size_t index)
    {
        std::stringstream stream;
        stream << std::setfill ('0') << std::setw(32*2) << std::hex << index;
        return stream.str();
    }


    //get_key_part(uint256)
    //curl --data '{"jsonrpc":"2.0","method":"eth_call","params":[{"to":"0x492CCD1eAeCDA0262e3AAeF8445F3F731a30AfbB","data": "0x96ce93e2+_INDEX" },"latest"],"id":1}' https://ropsten.infura.io
    std::string
    get_public_pem_chunk(size_t index, const std::string& url = ROPSTEN_URL)
    {
        std::string function_call{GET_KEY_CHUNK};
        function_call.append(integer_to_32_byte_hex(index));
        const auto response = get_curl_response(function_call, url);
        // need to clean up the response a bit, remove the prepended "0x"
        return response["result"].asString().substr(2);
    }


    std::string
    insert_line_breaks(const std::string& in_string, const size_t line_length=64)
    {
        std::stringstream ss;
        for(size_t i=0 ; i < in_string.size(); ++i)
        {
            if ( 0==i % line_length)
            {
                ss << '\n';
            }
            ss << in_string[i];
        }
        return ss.str();
    }


    // TODO rename hex_string_to_string
    std::string
    hex_string_to_string(const std::string& hex_string)
    {
        std::string public_key;
        for(size_t i=0 ; i < hex_string.size(); i+=2)
        {
            public_key += static_cast<unsigned char>(std::strtoul(hex_string.substr(i,2).c_str(), nullptr, 16));
        }
        return insert_line_breaks(public_key);
    }
}


namespace bzn::utils::crypto
{
    std::string
    retrieve_bluzelle_public_key_from_contract()
    {
        std::string public_key_hex;
        // TODO retrieve the public key from the contract

        for (size_t index=0; index < get_public_pem_size(); ++index)
        {
            const auto line = get_public_pem_chunk(index);
            public_key_hex += line;
        }

        std::string public_key{"-----BEGIN PUBLIC KEY-----"};
        public_key.append(hex_string_to_string(public_key_hex));
        public_key.append("\n-----END PUBLIC KEY-----");
        return public_key;
    }


    int
    base64_decode(const std::string& base64_message, std::vector<unsigned char>& decoded_message)
    {
        if (base64_message.empty())
        {
            LOG(error) << "Input string to base64_decode must not be empty";
            return 1;
        }

        const size_t decoded_length = calculate_decode_length(base64_message);
        decoded_message.resize(decoded_length);

        // I have chosen to *not* use unique pointers for bio and b64 here, as the memory handling
        // is completely self contained within this function.

        // new_mem_buf requires a non const void*, I needed to copy the const base64_message into a
        // non const string so that gcc C++ wouldn't complain. The alternative, using const_cast
        // didn't seem right as I am not completely sure that new_mem_buf doesn't modify the input,
        // it probably doesn't.
        std::string nonconst{base64_message};
        BIO* bio = BIO_new_mem_buf(nonconst.data(), -1);

        BIO* b64 = BIO_new(BIO_f_base64());
        bio = BIO_push(b64, bio);

        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer

        const int length = BIO_read(bio, decoded_message.data(), static_cast<int>(base64_message.size()));

        // free all is used here as b64 was pushed onto bio making it a BIO chain.
        BIO_free_all(bio);

        if (static_cast<size_t>(length) != decoded_length) //length should equal decoded_length, else something went horribly wrong
        {
            LOG(error) << "base64_decode failed to decode a base64 encoded string";
            return (1); // failure
        }
        return (0); //success
    }


    bool
    verify_signature(const std::string& public_key, const std::string& signature, const std::string& uuid)
    {
        if (public_key.empty() || signature.empty() || uuid.empty())
        {
            LOG(error) << "Unable to verify the signature as one or more of the public key, signature or uuid strings were empty";
            return false;
        }

        auto public_rsa = create_public_RSA(public_key);
        if (nullptr==public_rsa)
        {
            LOG(error) << "Unable to create RSA from public key while validating the node.";
            return false;
        }

        std::vector<unsigned char> binary_signature;
        base64_decode(signature, binary_signature);

        bool authentic{false};
        bool ret_val = RSA_verify_signature( public_rsa, binary_signature, uuid, authentic);

        return ret_val & authentic;
    }


    std::string read_pem_file(const std::string& filename, const std::string& expected_type)
    {
        char* name;
        char* headers;
        unsigned char* data;
        long len;

        ::FILE* fp = fopen(filename.c_str(), "r");
        if (!fp)
        {
            throw std::runtime_error("Failed to read pem file: " + filename);
        }

        PEM_read(fp, &name, &headers, &data, &len);
        ::fclose(fp);

        if (std::string(name) != expected_type)
        {
            throw std::runtime_error("Expected to find a " + expected_type + " in " + filename + ", but found a " + std::string(name));
        }

        std::string result(reinterpret_cast<char const*>(data), len);

        OPENSSL_free(name);
        OPENSSL_free(data);
        OPENSSL_free(headers);

        return result;
    }
}
