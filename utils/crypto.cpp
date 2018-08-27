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

// https://eclipsesource.com/blogs/2016/09/07/tutorial-code-signing-and-verification-with-openssl/

#include "crypto.hpp"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <iostream>
#include <boost/beast/core/detail/base64.hpp>
#include <fstream>

namespace {

    /*!
     * This function is used by base_64_decode to determine the amount of memory required
     * for the decoded string.
     * @param base_64_input
     * @return a size_t containing the number of bytes expected in the decoded string.
    */
    size_t calculate_decode_length(const char* base_64_input)
    {
        const auto len{strlen(base_64_input)};
        auto padding = (base_64_input[len - 1] == '=' && base_64_input[len - 2] == '=') ? 2 : 1;
        return (len * 3) / 4 - padding;
    }

    RSA* create_public_RSA(const std::string& key)
    {
        RSA *rsa{NULL};
        BIO *keybio;
        const char* c_string = key.c_str();
        keybio = BIO_new_mem_buf((void*)c_string, -1);
        if (keybio==NULL) {
            return 0;
        }
        rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
        return rsa;
    }
};


namespace bzn::utils::crypto
{
    int base_64_decode(const char *base64_message, unsigned char **buffer, size_t *length)
    {
        BIO *bio;
        BIO *b64;
        const auto decodeLen = calculate_decode_length(base64_message);
        *buffer = (unsigned char*)malloc(decodeLen + 1);
        (*buffer)[decodeLen] = '\0';

        bio = BIO_new_mem_buf(base64_message, -1);
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_push(b64, bio);

        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
        *length = BIO_read(bio, *buffer, strlen(base64_message));

        assert(*length == decodeLen); //length should equal decodeLen, else something went horribly wrong
        if(*length != decodeLen)
            throw std::runtime_error("Unable to decode base64 encoded string.");

        BIO_free_all(bio);

        return (0); //success
    }



    bool RSA_verify_signature( const RSA* rsa,
                               const unsigned char* msg_hash,
                               size_t msg_hash_length,
                               const char* message,
                               size_t message_length,
                               bool* is_authentic)
    {
        *is_authentic = false;
        EVP_PKEY* public_key  = EVP_PKEY_new();
        EVP_PKEY_assign_RSA(public_key, rsa);
        EVP_MD_CTX* m_RSAVerifyCtx = EVP_MD_CTX_create();

        const EVP_MD* md = EVP_sha256();//   EVP_get_digestbyname( "SHA256" ); // EVP_sha256()
        EVP_DigestInit_ex( m_RSAVerifyCtx, md, NULL );

        if (EVP_DigestVerifyInit(m_RSAVerifyCtx,NULL, md, NULL,public_key)<=0) {
            return false;
        }
        if (EVP_DigestVerifyUpdate(m_RSAVerifyCtx, message, message_length) <= 0) {
            return false;
        }
        int AuthStatus = EVP_DigestVerifyFinal(m_RSAVerifyCtx, msg_hash, msg_hash_length);
        if (AuthStatus==1) {
            *is_authentic = true;
            EVP_MD_CTX_cleanup(m_RSAVerifyCtx);
            return true;
        } else if(AuthStatus==0){
            unsigned long err_number = ERR_get_error();
            char buffer[1024]{0};
            ERR_error_string_n(err_number, buffer, 1024);
            *is_authentic = false;
            EVP_MD_CTX_cleanup(m_RSAVerifyCtx);
            return true;
        } else{
            *is_authentic = false;
            EVP_MD_CTX_cleanup(m_RSAVerifyCtx);
            return false;
        }
    }


    //verify(signature(UUID), BLZ public key)
    // convert base64 encoded signature  to bin
    //      openssl base64 -d -in signature.txt -out /tmp/sign.sha256
    // verify the uuid
    //      openssl dgst -sha256 -verify public.pem -signature /tmp/sign.sha256 uuid.txt
    bool verify_signature(const std::string& public_key, const std::string& signature, const std::string& uuid)
    {
        RSA* public_rsa = create_public_RSA(public_key);
        if(NULL==public_rsa)
        {
            throw std::runtime_error("Unable to create RSA object from public key");
        }

        size_t length = 0;
        unsigned char* base64DecodeOutput{0};
        base_64_decode(signature.c_str(), &base64DecodeOutput, &length);

        bool authentic{false};

        bool ret_val = RSA_verify_signature( public_rsa,
                                             base64DecodeOutput,
                                             length,
                                             (char*)uuid.c_str(),
                                             uuid.size(),
                                             &authentic);
        return ret_val & authentic;
    }

}

