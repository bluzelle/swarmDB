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
#include <mocks/mock_options_base.hpp>
#include <options/options.hpp>
#include <gtest/gtest.h>
#include <proto/bluzelle.pb.h>
#include <fstream>
#include <boost/range/irange.hpp>

using namespace ::testing;

class crypto_test : public Test
{
public:
    std::shared_ptr<bzn::options_base> options = std::make_shared<bzn::options>();
    std::shared_ptr<bzn::crypto> crypto;

    const std::string private_key_file = "test_private_key.pem";
    const std::string public_key_file = "test_public_key.pem";

    const std::string test_private_key_pem =
            "-----BEGIN EC PRIVATE KEY-----\n"
            "MHQCAQEEIFskFIUF/rSbcA62nW+a90ptWSaMnwlpNa6w5ab2BjT3oAcGBSuBBAAK\n"
            "oUQDQgAEIUn235kRQyjEwZiexq7tOSu/9QiabDqg8Mcwr7lq+Hi7/xx7A37wZBHV\n"
            "tCRpaXbJNqRhIErf6FnOZI3m71sQoA==\n"
            "-----END EC PRIVATE KEY-----\n";

    const std::string test_public_key_pem =
            "-----BEGIN PUBLIC KEY-----\n"
            "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEIUn235kRQyjEwZiexq7tOSu/9QiabDqg\n"
            "8Mcwr7lq+Hi7/xx7A37wZBHVtCRpaXbJNqRhIErf6FnOZI3m71sQoA==\n"
            "-----END PUBLIC KEY-----\n";

    wrapped_bzn_msg msg;

    crypto_test()
    {
        this->msg.set_payload("pretend this is a serialized protobuf message");

        std::ofstream ofile(private_key_file.c_str());
        ofile << test_private_key_pem;
        ofile.close();

        std::ofstream ofile2(public_key_file.c_str());
        ofile2 << test_public_key_pem;
        ofile2.close();

        this->options->get_mutable_simple_options().set(bzn::option_names::NODE_PRIVATEKEY_FILE, private_key_file);
        this->options->get_mutable_simple_options().set(bzn::option_names::NODE_PUBKEY_FILE, public_key_file);

        this->crypto = std::make_shared<bzn::crypto>(this->options);

    }

    ~crypto_test()
    {
        ::unlink(private_key_file.c_str());
        ::unlink(public_key_file.c_str());
    }

};

TEST_F(crypto_test, messages_use_my_public_key)
{
    EXPECT_TRUE(crypto->sign(msg));
    EXPECT_EQ(msg.sender(), this->options->get_uuid());
}

TEST_F(crypto_test, messages_signed_and_verified)
{
    wrapped_bzn_msg msg2 = msg;
    wrapped_bzn_msg msg3 = msg;

    EXPECT_TRUE(crypto->sign(msg));
    EXPECT_TRUE(crypto->verify(msg));
}

TEST_F(crypto_test, bad_signature_caught)
{
    wrapped_bzn_msg msg2 = msg;

    EXPECT_TRUE(crypto->sign(msg));

    msg2.set_signature("a" + msg.signature());
    EXPECT_FALSE(crypto->verify(msg2));
}

TEST_F(crypto_test, bad_sender_caught)
{
    wrapped_bzn_msg msg3 = msg;

    EXPECT_TRUE(crypto->sign(msg));

    msg3.set_sender('a' + msg.sender());
    msg3.set_signature(msg.signature());
    EXPECT_FALSE(crypto->verify(msg3));
}

TEST_F(crypto_test, hash_no_collision)
{
    /*
     * It's well outside our scope to test that the hash actually has cryptographically secure hash properties,
     * so we'll just test that it "looks like" a hash function
     */

    std::set<std::string> hash_values;
    std::string hash_data = "";

    for(int i=0; i<10000; i++)
    {
        hash_data.push_back('A');
    }

    for(int i=0; i<1000; i++)
    {
        hash_data[i] = 'a' + (i%31);
        hash_values.insert(this->crypto->hash(hash_data));
    }

    EXPECT_EQ(hash_values.size(), 1000u);
}

TEST_F(crypto_test, hash_deterministic_but_not_identity)
{
    for(int i=0; i<1000; i++)
    {
        std::string str = std::to_string(i);
        EXPECT_EQ(this->crypto->hash(str), this->crypto->hash(str));
        EXPECT_NE(str, this->crypto->hash(str));
    }
}
