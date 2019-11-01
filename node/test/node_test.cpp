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

#include <node/node.hpp>
#include <mocks/mock_boost_asio_beast.hpp>

#include <gmock/gmock.h>
#include <include/bluzelle.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_chaos_base.hpp>
#include <mocks/mock_monitor.hpp>
#include <mocks/smart_mock_io.hpp>

#include <options/options.hpp>
#include <chaos/chaos.hpp>
#include <crypto/crypto.hpp>

#include <proto/bluzelle.pb.h>
#include <proto/pbft.pb.h>

#include <thread>
#include <chrono>

using namespace ::testing;

namespace
{
    const auto TEST_ENDPOINT =
        boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string("127.0.0.1"), 0}; // any port
    const auto TEST_ENDPOINT_2 =
            boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string("127.0.0.2"), 0}; // any port
}

class node_test2 : public Test
{
public:
    std::shared_ptr<bzn::mock_chaos_base> mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
    std::shared_ptr<bzn::options_base> options = std::make_shared<bzn::options>();
    std::shared_ptr<bzn::crypto_base> crypto = std::shared_ptr<bzn::crypto>();
    std::shared_ptr<bzn::mock_monitor> monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();
    std::shared_ptr<bzn::asio::smart_mock_io> mock_io = std::make_shared<bzn::asio::smart_mock_io>();
    std::shared_ptr<bzn::node_base> node = std::make_shared<bzn::node>(mock_io, mock_io->websocket, mock_chaos, TEST_ENDPOINT, crypto, options, monitor);

    bzn_envelope db_msg;
    uint callback_invoked = 0;

    node_test2()
    {
        db_msg.set_database_msg("hihi");
        this->node->register_for_message(bzn_envelope::kDatabaseMsg, [&](auto, auto){callback_invoked++;});
    }

    ~node_test2()
    {
        this->mock_io->shutdown();
    }
};

namespace  bzn
{

    TEST(node, test_that_node_constructed_with_invalid_address_throws)
    {
        auto io_context = std::make_shared<bzn::asio::io_context>();
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto options = std::shared_ptr<bzn::options>();
        auto crypto = std::shared_ptr<bzn::crypto>();
        std::shared_ptr<bzn::mock_monitor> monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();

        EXPECT_THROW(
            bzn::node(io_context, nullptr, mock_chaos,
                boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string("8.8.8.8"), 8080}, crypto, options, monitor),
            std::exception
        );
    }

    TEST_F(node_test2, test_accept_incoming_connection)
    {
        this->node->start(nullptr);

        EXPECT_EQ(mock_io->socket_count, 1u);

        mock_io->do_incoming_connection(0);
        mock_io->ws_read_closures.at(0)(this->db_msg.SerializeAsString());
        this->mock_io->yield_until_clear();
        EXPECT_EQ(callback_invoked, 1u);
    }

    TEST_F(node_test2, test_make_new_connection)
    {
        this->node->start(nullptr);

        this->node->send_message_str(TEST_ENDPOINT, std::make_shared<std::string>("test string"));

        this->mock_io->yield_until_clear();
        EXPECT_EQ(this->mock_io->ws_write_closures.at(1)(), "test string");
    }

    TEST_F(node_test2, test_reuse_connection)
    {
        this->node->start(nullptr);

        this->node->send_message_str(TEST_ENDPOINT, std::make_shared<std::string>("test string"));
        this->mock_io->yield_until_clear();
        EXPECT_EQ(this->mock_io->ws_write_closures.at(1)(), "test string");

        this->node->send_message_str(TEST_ENDPOINT, std::make_shared<std::string>("test string2"));
        this->mock_io->yield_until_clear();
        EXPECT_EQ(this->mock_io->ws_write_closures.at(1)(), "test string2");

        this->mock_io->yield_until_clear();
        EXPECT_EQ(mock_io->socket_count, 2u);
    }

    TEST_F(node_test2, new_session_for_new_ep)
    {
        this->node->start(nullptr);

        this->node->send_message_str(TEST_ENDPOINT, std::make_shared<std::string>("test A"));
        this->mock_io->yield_until_clear();
        EXPECT_EQ(this->mock_io->ws_write_closures.at(1)(), "test A");

        this->node->send_message_str(TEST_ENDPOINT_2, std::make_shared<std::string>("test B"));
        this->mock_io->yield_until_clear();
        EXPECT_EQ(this->mock_io->ws_write_closures.at(2)(), "test B");

        this->mock_io->yield_until_clear();
        EXPECT_EQ(mock_io->socket_count, 3u);
    }

    TEST_F(node_test2, DISABLED_test_replace_dead_session)
    {
        this->node->start(nullptr);

        this->node->send_message_str(TEST_ENDPOINT, std::make_shared<std::string>("test string"));
        this->mock_io->yield_until_clear();
        EXPECT_EQ(this->mock_io->ws_write_closures.at(1)(), "test string");

        // kill connection
        this->mock_io->socket_is_open[1] = false;

        this->node->send_message_str(TEST_ENDPOINT, std::make_shared<std::string>("test string2"));
        this->mock_io->yield_until_clear();
        EXPECT_EQ(this->mock_io->ws_write_closures.at(2)(), "test string2");

        this->mock_io->yield_until_clear();
        EXPECT_EQ(mock_io->socket_count, 3u);
    }

    TEST(node, test_that_registering_message_handler_can_only_be_done_once)
    {
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto mock_io_context = std::make_shared<NiceMock<bzn::asio::mock_io_context_base>>();
        auto options = std::make_shared<bzn::options>();
        auto crypto = std::shared_ptr<bzn::crypto>();
        auto monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();

        auto node = std::make_shared<bzn::node>(mock_io_context, nullptr, mock_chaos, TEST_ENDPOINT, crypto, options, monitor);

        // test that nulls are rejected...
        ASSERT_FALSE(node->register_for_message(bzn_envelope::kDatabaseMsg, nullptr));

        // test that non-null handler is added...
        ASSERT_TRUE(node->register_for_message(bzn_envelope::kDatabaseMsg, [](const auto&, auto){}));

        // test that we can't overwrite previous registration...
        ASSERT_FALSE(node->register_for_message(bzn_envelope::kDatabaseMsg, [](const auto&, auto){}));
    }

    TEST(node, test_that_node_doesnt_call_error_handler_on_successful_connect)
    {
        std::shared_ptr<bzn::mock_chaos_base> mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        std::shared_ptr<bzn::options_base> options = std::make_shared<bzn::options>();
        std::shared_ptr<bzn::crypto_base> crypto = std::shared_ptr<bzn::crypto>();
        std::shared_ptr<bzn::mock_monitor> monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();
        std::shared_ptr<bzn::asio::smart_mock_io> mock_io = std::make_shared<bzn::asio::smart_mock_io>();
        std::shared_ptr<bzn::node_base> node = std::make_shared<bzn::node>(mock_io, mock_io->websocket, mock_chaos, TEST_ENDPOINT, crypto, options, monitor);

        bool called{false};
        node->register_error_handler([&called](auto &/*ep*/, auto& ec)
        {
            ASSERT_NE(ec, boost::system::error_code{});
            called = true;
        });

        node->send_message_str(TEST_ENDPOINT_2, std::make_shared<std::string>("test string"));
        mock_io->yield_until_clear();
        ASSERT_EQ(called, false);
        mock_io->shutdown();
    }

    TEST(node, test_that_node_calls_error_handler_on_no_connect)
    {
        std::shared_ptr<bzn::mock_chaos_base> mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        std::shared_ptr<bzn::options_base> options = std::make_shared<bzn::options>();
        std::shared_ptr<bzn::crypto_base> crypto = std::shared_ptr<bzn::crypto>();
        std::shared_ptr<bzn::mock_monitor> monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();
        std::shared_ptr<bzn::asio::smart_mock_io> mock_io = std::make_shared<bzn::asio::smart_mock_io>();
        std::shared_ptr<bzn::node_base> node = std::make_shared<bzn::node>(mock_io, mock_io->websocket, mock_chaos, TEST_ENDPOINT, crypto, options, monitor);

        bool called{false};
        node->register_error_handler([&called](auto &/*ep*/, auto& ec)
        {
            ASSERT_NE(ec, boost::system::error_code{});
            called = true;
        });

        mock_io->tcp_connect_works = false;
        node->send_message_str(TEST_ENDPOINT_2, std::make_shared<std::string>("test string"));
        mock_io->yield_until_clear();
        ASSERT_EQ(called, true);
        mock_io->shutdown();
    }

#if 0
    TEST(node, test_that_wrongly_signed_messages_are_dropped)
    {
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto mock_io_context = std::make_shared<NiceMock<bzn::asio::mock_io_context_base>>();
        auto options = std::make_shared<bzn::options>();
        options->get_mutable_simple_options().set(bzn::option_names::CRYPTO_ENABLED_INCOMING, "true");
        auto monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();
        auto crypto = std::make_shared<bzn::crypto>(options, monitor);
        auto mock_session = std::make_shared<bzn::mock_session_base>();
        auto node = std::make_shared<bzn::node>(mock_io_context, nullptr, mock_chaos, TEST_ENDPOINT, crypto, options, monitor);

        // Add our test callback...
        unsigned int callback_execute = 0u;
        node->register_for_message(bzn_envelope::kPbft, [&](const auto& /*msg*/, auto)
        {
            callback_execute++;
        });

        bzn_envelope bad_msg;
        bad_msg.set_pbft("some stuff");
        bad_msg.set_sender("Elizabeth the Second, by the Grace of God of the United Kingdom, Canada and Her other Realms and Territories Queen, Head of the Commonwealth, Defender of the Faith");
        bad_msg.set_signature("probably not a valid signature");

        bzn_envelope anon_msg;
        anon_msg.set_pbft("some stuff");
        anon_msg.set_sender("");
        anon_msg.set_signature("");

        node->priv_protobuf_handler(bad_msg, mock_session);
        EXPECT_EQ(callback_execute, 0u);

        node->priv_protobuf_handler(anon_msg, mock_session);
        EXPECT_EQ(callback_execute, 1u);
    }
#endif
} // namespace bzn
