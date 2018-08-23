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


std::string base_64_decode(const std::string& in_text)
{
    std::string cleaned_in_text{""};
    std::copy_if(in_text.begin(), in_text.end(), std::back_inserter(cleaned_in_text), [](auto c) { return c != '\n'; });
    return boost::beast::detail::base64_decode(cleaned_in_text);
}




RSA* create_public_RSA(const std::string& key)
{
    RSA *rsa = NULL;
    BIO *keybio;
    const char* c_string = key.c_str();
    keybio = BIO_new_mem_buf((void*)c_string, -1);
    if (keybio==NULL) {
        return 0;
    }
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);


    return rsa;
}


bool RSA_verify_signature( RSA* rsa,
                         unsigned char* MsgHash,
                         size_t MsgHashLen,
                         const char* Msg,
                         size_t MsgLen,
                         bool* Authentic) {
    *Authentic = false;
    EVP_PKEY* pubKey  = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(pubKey, rsa);
    EVP_MD_CTX* m_RSAVerifyCtx = EVP_MD_CTX_create();

    if (EVP_DigestVerifyInit(m_RSAVerifyCtx,NULL, EVP_sha256(),NULL,pubKey)<=0) {
        return false;
    }
    if (EVP_DigestVerifyUpdate(m_RSAVerifyCtx, Msg, MsgLen) <= 0) {
        return false;
    }
    int AuthStatus = EVP_DigestVerifyFinal(m_RSAVerifyCtx, MsgHash, MsgHashLen);
    if (AuthStatus==1) {
        *Authentic = true;
        EVP_MD_CTX_cleanup(m_RSAVerifyCtx);
        return true;
    } else if(AuthStatus==0){




        std::cout << "  ** error: [" << ERR_get_error() << "] " << std::endl;


        *Authentic = false;
        EVP_MD_CTX_cleanup(m_RSAVerifyCtx);
        return true;
    } else{
        *Authentic = false;
        EVP_MD_CTX_cleanup(m_RSAVerifyCtx);
        return false;
    }
}



int spc_verify(unsigned char* msg,
        unsigned int mlen,
        unsigned char* sig,
        unsigned int siglen,
        RSA* r
        )
{
    unsigned char  hash[20];
    BN_CTX* c{0};
    int ret;

    if (!(BN_CTX_new()))
        return 0;
    if(SHA1(msg, mlen, hash) || !RSA_blinding_on(r, c))
    {
        BN_CTX_free(c);
        return 0;
    }

    ret = RSA_verify(NID_sha1, hash, 20, sig, siglen, r);
    RSA_blinding_off(r);
    BN_CTX_free(c);
    return ret;

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
    //std::string decoded_signature = base_64_decode(signature);
    std::string decoded_signature = boost::beast::detail::base64_decode(signature);





    bool authentic{false};

    bool ret_val = RSA_verify_signature( public_rsa,
            (unsigned char*)decoded_signature.c_str(),
            decoded_signature.size(),
            (char*)uuid.c_str(),
            uuid.size(),
            &authentic);

    //bool ret_val = spc_verify((unsigned char*)uuid.c_str(), uuid.size(), (unsigned char*)signature.c_str(), signature.size(), public_rsa);

//
//
//    bool result = RSA_verify_signature(public_key, encoded_signature, encoded_signature.size(), )

    // convert base64 encoded signature  to bin
    //      openssl base64 -d -in signature.txt -out /tmp/sign.sha256
    // verify the uuid
    //      openssl dgst -sha256 -verify public.pem -signature /tmp/sign.sha256 uuid.txt


    //std::cout << "ret_val: " << ret_val << "\npublic_rsa:" << public_rsa << "\n and \n" << public_key << " and " << signature << " and " << uuid << std::endl;
    return ret_val & authentic;
}







/*
RSA* create_private_RSA(const std::string& key)
{
    RSA* rsa{NULL};
    const char* c_string = key.c_str();
    BIO* keybio = BIO_new_mem_buf((void*) c_string, -1);
    if (keybio==NULL)
    {
        return 0;
    }
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
    return rsa;
}


bool RSASign( RSA* rsa,
              const unsigned char* Msg,
              size_t MsgLen,
              unsigned char** EncMsg,
              size_t* MsgLenEnc)
{
    EVP_MD_CTX* m_RSASignCtx = EVP_MD_CTX_create();
    EVP_PKEY* priKey  = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(priKey, rsa);
    if (EVP_DigestSignInit(m_RSASignCtx,NULL, EVP_sha256(), NULL,priKey)<=0) {
        return false;
    }
    if (EVP_DigestSignUpdate(m_RSASignCtx, Msg, MsgLen) <= 0) {
        return false;
    }
    if (EVP_DigestSignFinal(m_RSASignCtx, NULL, MsgLenEnc) <=0) {
        return false;
    }
    *EncMsg = (unsigned char*)malloc(*MsgLenEnc);
    if (EVP_DigestSignFinal(m_RSASignCtx, *EncMsg, MsgLenEnc) <= 0) {
        return false;
    }
    EVP_MD_CTX_cleanup(m_RSASignCtx);
    return true;
}
 */