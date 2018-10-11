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

#include <crypto/crypto.hpp>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

using namespace bzn;

namespace
{
    const std::string PEM_PREFIX = "-----BEGIN PUBLIC KEY-----\n";
    const std::string PEM_SUFFIX = "\n-----END PUBLIC KEY-----\n";
}

crypto::crypto(std::shared_ptr<bzn::options_base> options)
        : options(std::move(options))
{
}

void
crypto::start()
{
    LOG(info) << "Using " << SSLeay_version(SSLEAY_VERSION);
    if(this->options->get_simple_options().get<bool>(bzn::option_names::CRYPTO_ENABLED_OUTGOING))
    {
        this->load_private_key();
    }
}

bool
crypto::verify(const wrapped_bzn_msg& msg)
{
    BIO* bio = NULL;
    EC_KEY* pubkey = NULL;
    EVP_PKEY* key = NULL;
    EVP_MD_CTX* context = NULL;

    // In openssl 1.0.1 (but not newer versions), EVP_DigestVerifyFinal strangely expects the signature as
    // a non-const pointer.
    std::string signature = msg.signature();
    char* sig_ptr = signature.data();

    // This is admittedly a weird structure, but it seems the cleanest way to deal with the combined manual
    // memory management and error handling

    bool result =
            // Reconstruct the PEM file in memory (this is awkward, but it avoids dealing with EC specifics)
            (bio = BIO_new(BIO_s_mem()))
            && (0 < BIO_write(bio, PEM_PREFIX.c_str(), PEM_PREFIX.length()))
            && (0 < BIO_write(bio, msg.sender().c_str(), msg.sender().length()))
            && (0 < BIO_write(bio, PEM_SUFFIX.c_str(), PEM_SUFFIX.length()))

            // Parse the PEM string to get the public key the message is allegedly from
            && (pubkey = PEM_read_bio_EC_PUBKEY(bio, NULL, NULL, NULL))
            && (1 == EC_KEY_check_key(pubkey))
            && (key = EVP_PKEY_new())
            && (1 == EVP_PKEY_set1_EC_KEY(key, pubkey))

            // Perform the signature validation
            && (context = EVP_MD_CTX_create())
            && (1 == EVP_DigestVerifyInit(context, NULL, EVP_sha512(), NULL, key))
            && (1 == EVP_DigestVerifyUpdate(context, msg.payload().c_str(), msg.payload().length()))
            && (1 == EVP_DigestVerifyFinal(context, reinterpret_cast<unsigned char*>(sig_ptr), msg.signature().length()));

    if (context) EVP_MD_CTX_destroy(context);
    if (pubkey) OPENSSL_free(pubkey);
    if (key) OPENSSL_free(key);
    if (bio) OPENSSL_free(bio);

    /* Any errors here can be attributed to a bad (potentially malicious) incoming message, and we we should not
     * pollute our own logs with them (but we still have to clear the error state)
     */
    ERR_clear_error();


    return result;
}

bool
crypto::sign(wrapped_bzn_msg& msg)
{
    if (msg.sender().empty())
    {
        msg.set_sender(this->options->get_uuid());
    }

    if (msg.sender() != options->get_uuid())
    {
        LOG(error) << "Cannot sign message purportedly sent by " << msg.sender();
        return false;
    }

    EVP_MD_CTX* context = NULL;
    size_t signature_length = 0;
    unsigned char* signature = NULL;

    bool result =
            (bool) (context = EVP_MD_CTX_create())
            && (1 == EVP_DigestSignInit(context, NULL, EVP_sha512(), NULL, this->private_key_EVP))
            && (1 == EVP_DigestSignUpdate(context, msg.payload().c_str(), msg.payload().size()))
            && (1 == EVP_DigestSignFinal(context, NULL, &signature_length))
            && (bool) (signature = (unsigned char*) OPENSSL_malloc(sizeof(unsigned char) * signature_length))
            && (1 == EVP_DigestSignFinal(context, signature, &signature_length));

    if (result)
    {
        msg.set_signature(signature, signature_length);
    }
    else
    {
        LOG(error) << "Failed to sign message with openssl error (do we have a valid private key?)";
        // Message will be sent without signature; depending on settings they may still accept it
    }

    if (context) EVP_MD_CTX_destroy(context);
    if (signature) OPENSSL_free(signature);
    this->log_openssl_errors();

    return result;
}

bool
crypto::load_private_key()
{
    auto filename = this->options->get_simple_options().get<std::string>(bzn::option_names::NODE_PRIVATEKEY_FILE);
    ::FILE* fp;

    bool result =
            (fp = fopen(filename.c_str(), "r"))
            && (this->private_key_EC = PEM_read_ECPrivateKey(fp, NULL, NULL, NULL))
            && (1 == EC_KEY_check_key(this->private_key_EC))
            && (this->private_key_EVP = EVP_PKEY_new())
            && (1 == EVP_PKEY_set1_EC_KEY(this->private_key_EVP, this->private_key_EC));

    if (!result)
    {
        LOG(error) << "Crypto failed to load private key; will not be able to sign messages";
    }

    if (fp) fclose(fp);
    this->log_openssl_errors();

    return result;
}

void
crypto::log_openssl_errors()
{
    unsigned long last_error;
    char buffer[120]; //openssl says this is the appropriate length

    while((last_error = ERR_get_error()))
    {
        ERR_error_string(last_error, buffer);
        LOG(error) << buffer;
    }
}

crypto::~crypto()
{
    if (this->private_key_EC) OPENSSL_free(this->private_key_EC);
    if (this->private_key_EVP) EVP_PKEY_free(this->private_key_EVP);
}
