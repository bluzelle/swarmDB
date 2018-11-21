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
    LOG(info) << "Using " << SSLeay_version(SSLEAY_VERSION);
    if(this->options->get_simple_options().get<bool>(bzn::option_names::CRYPTO_ENABLED_OUTGOING))
    {
        this->load_private_key();
    }
}

const std::string&
crypto::extract_payload(const bzn_envelope& msg)
{
    switch (msg.payload_case())
    {
        case bzn_envelope::kDatabaseMsg :
        {
            return msg.database_msg();
        }
        case bzn_envelope::kPbftInternalRequest :
        {
            return msg.pbft_internal_request();
        }
        case bzn_envelope::kDatabaseResponse :
        {
            return msg.database_response();
        }
        case bzn_envelope::kJson :
        {
            return msg.json();
        }
        case bzn_envelope::kAudit :
        {
            return msg.audit();
        }
        case bzn_envelope::kPbft :
        {
            return msg.pbft();
        }
        case bzn_envelope::kPbftMembership :
        {
            return msg.pbft_membership();
        }
        default :
        {
            throw std::runtime_error(
                    "Crypto does not know how to handle a message with type " + std::to_string(msg.payload_case()));
        }
    }
}

const std::string
crypto::deterministic_serialize(const bzn_envelope& msg)
{
    // I don't like hand-rolling this, but doing it here lets us do it in one place, while avoiding implementing
    // it for every message type

    std::vector<std::string> tokens = {msg.sender(), std::to_string(msg.payload_case()), this->extract_payload(msg), std::to_string(msg.timestamp())};

    // this construction defeats an attack where the adversary blurs the lines between the fields. if we simply
    // concatenate the fields, then consider the two messages
    //     {sender: "foo", payload: "bar"}
    //     {sender: "foobar", payload: ""}
    // they may have the same serialization - and therefore the same signature.
    std::string result = "";
    for (const auto& token : tokens)
    {
        result += (std::to_string(token.length()) + "|" + token);
    }

    return result;
}

bool
crypto::verify(const bzn_envelope& msg)
{
    BIO_ptr_t bio(BIO_new(BIO_s_mem()), &BIO_free);
    EC_KEY_ptr_t pubkey(nullptr, &EC_KEY_free);
    EVP_PKEY_ptr_t key(EVP_PKEY_new(), &EVP_PKEY_free);
    EVP_MD_CTX_ptr_t context(EVP_MD_CTX_create(), &EVP_MD_CTX_free);

    if (!bio || !key || !context)
    {
        LOG(error) << "failed to allocate memory for signature verification";
        return false;
    }

    const auto msg_text = this->deterministic_serialize(msg);

    // In openssl 1.0.1 (but not newer versions), EVP_DigestVerifyFinal strangely expects the signature as
    // a non-const pointer.
    std::string signature = msg.signature();
    char* sig_ptr = signature.data();

    bool result =
            // Reconstruct the PEM file in memory (this is awkward, but it avoids dealing with EC specifics)
            (0 < BIO_write(bio.get(), PEM_PREFIX.c_str(), PEM_PREFIX.length()))
            && (0 < BIO_write(bio.get(), msg.sender().c_str(), msg.sender().length()))
            && (0 < BIO_write(bio.get(), PEM_SUFFIX.c_str(), PEM_SUFFIX.length()))

            // Parse the PEM string to get the public key the message is allegedly from
            && (pubkey = EC_KEY_ptr_t(PEM_read_bio_EC_PUBKEY(bio.get(), NULL, NULL, NULL), &EC_KEY_free))
            && (1 == EC_KEY_check_key(pubkey.get()))
            && (1 == EVP_PKEY_set1_EC_KEY(key.get(), pubkey.get()))

            // Perform the signature validation
            && (1 == EVP_DigestVerifyInit(context.get(), NULL, EVP_sha512(), NULL, key.get()))
            && (1 == EVP_DigestVerifyUpdate(context.get(), msg_text.c_str(), msg_text.length()))
            && (1 == EVP_DigestVerifyFinal(context.get(), reinterpret_cast<unsigned char*>(sig_ptr), msg.signature().length()));

    /* Any errors here can be attributed to a bad (potentially malicious) incoming message, and we we should not
     * pollute our own logs with them (but we still have to clear the error state)
     */
    ERR_clear_error();

    return result;
}

bool
crypto::sign(bzn_envelope& msg)
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

    const auto msg_text = this->deterministic_serialize(msg);

    EVP_MD_CTX_ptr_t context(EVP_MD_CTX_create(), &EVP_MD_CTX_free);
    size_t signature_length = 0;

    bool result =
            (bool) (context)
            && (1 == EVP_DigestSignInit(context.get(), NULL, EVP_sha512(), NULL, this->private_key_EVP.get()))
            && (1 == EVP_DigestSignUpdate(context.get(), msg_text.c_str(), msg_text.length()))
            && (1 == EVP_DigestSignFinal(context.get(), NULL, &signature_length));

    auto deleter = [](unsigned char* ptr){OPENSSL_free(ptr);};
    std::unique_ptr<unsigned char, decltype(deleter)> signature((unsigned char*) OPENSSL_malloc(sizeof(unsigned char) * signature_length), deleter);

    result &=
            (bool) (signature)
            && (1 == EVP_DigestSignFinal(context.get(), signature.get(), &signature_length));

    if (result)
    {
        msg.set_signature(signature.get(), signature_length);
    }
    else
    {
        LOG(error) << "Failed to sign message with openssl error (do we have a valid private key?)";
        // Message will be sent without signature; depending on settings they may still accept it
    }

    this->log_openssl_errors();

    return result;
}

bool
crypto::load_private_key()
{
    auto filename = this->options->get_simple_options().get<std::string>(bzn::option_names::NODE_PRIVATEKEY_FILE);
    std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(filename.c_str(), "r"), &fclose);

    bool result =
            (bool) (fp)
            && (this->private_key_EC = EC_KEY_ptr_t(PEM_read_ECPrivateKey(fp.get(), NULL, NULL, NULL), &EC_KEY_free))
            && (1 == EC_KEY_check_key(this->private_key_EC.get()))
            && (this->private_key_EVP = EVP_PKEY_ptr_t(EVP_PKEY_new(), &EVP_PKEY_free))
            && (1 == EVP_PKEY_set1_EC_KEY(this->private_key_EVP.get(), this->private_key_EC.get()));

    if (!result)
    {
        LOG(error) << "Crypto failed to load private key; will not be able to sign messages";
    }

    this->log_openssl_errors();

    return result;
}

std::string
crypto::hash(const std::string& msg)
{
    EVP_MD_CTX_ptr_t context(EVP_MD_CTX_create(), &EVP_MD_CTX_free);
    size_t md_size = EVP_MD_size(EVP_sha512());

    auto deleter = [](unsigned char* ptr){OPENSSL_free(ptr);};
    std::unique_ptr<unsigned char, decltype(deleter)> hash_buffer((unsigned char*) OPENSSL_malloc(sizeof(unsigned char) * md_size), deleter);

    bool success =
            (bool) (context)
            && (1 == EVP_DigestInit_ex(context.get(), EVP_sha512(), NULL))
            && (1 == EVP_DigestUpdate(context.get(), msg.c_str(), msg.size()))
            && (1 == EVP_DigestFinal_ex(context.get(), hash_buffer.get(), NULL));

    if(!success)
    {
        this->log_openssl_errors();
        throw std::runtime_error(std::string("\nfailed to compute message hash ") + msg);
    }

    return std::string(reinterpret_cast<char*>(hash_buffer.get()), md_size);
}

std::string
crypto::hash(const bzn_envelope& msg)
{
    return this->hash(this->deterministic_serialize(msg));
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
