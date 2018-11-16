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

#include <options/options.hpp>
#include <chaos/chaos.hpp>
#include <crypto/crypto.hpp>

#include <proto/bluzelle.pb.h>

using namespace ::testing;

namespace
{
    const auto TEST_ENDPOINT =
        boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string("127.0.0.1"), 0}; // any port

}


namespace  bzn
{

    TEST(node, test_that_node_constructed_with_invalid_address_throws)
    {
        auto io_context = std::make_shared<bzn::asio::io_context>();
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto options = std::shared_ptr<bzn::options>();
        auto crypto = std::shared_ptr<bzn::crypto>();

        EXPECT_THROW(
            bzn::node(io_context, nullptr, mock_chaos, std::chrono::milliseconds(0),
                boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string("8.8.8.8"), 8080}, crypto, options),
            std::exception
        );
    }


    TEST(node, test_that_start_executes_do_accept_and_on_error_calls_it_again_and_success_creates_a_session)
    {
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_tcp_acceptor = std::make_unique<bzn::asio::Mocktcp_acceptor_base>();
        auto mock_websocket = std::make_shared<bzn::beast::Mockwebsocket_base>();
        auto mock_steady_timer = std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto options = std::shared_ptr<bzn::options>();
        auto crypto = std::shared_ptr<bzn::crypto>();

        // start expectations... (here we are returning a real socket)
        EXPECT_CALL(*mock_io_context, make_unique_tcp_socket()).WillRepeatedly(Invoke(
            []()
            {
                static boost::asio::io_context io;
                static boost::asio::ip::tcp::socket socket(io);
                auto mock_socket = std::make_unique<NiceMock<bzn::asio::Mocktcp_socket_base>>();
                EXPECT_CALL(*mock_socket, get_tcp_socket()).WillRepeatedly(ReturnRef(socket));
                return mock_socket;
            }));

        EXPECT_CALL(*mock_io_context, make_unique_strand());

        EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
            [&]()
            {
                return std::move(mock_steady_timer);
            }));

        // intercept the handler...
        bzn::asio::accept_handler accept_handler;
        EXPECT_CALL(*mock_tcp_acceptor, async_accept(_, _)).WillRepeatedly(Invoke(
            [&](auto& /*socket*/, auto handler)
            {
                accept_handler = handler;
            }));

        // return mock acceptor and test that TEST_ENDPOINT is passed...
        EXPECT_CALL(*mock_io_context, make_unique_tcp_acceptor(TEST_ENDPOINT)).WillOnce(Invoke(
            [&](auto& /*ep*/)
            {
                return std::move(mock_tcp_acceptor);
            }));

        // start the node...
        EXPECT_CALL(*mock_websocket, make_unique_websocket_stream(An<boost::asio::ip::tcp::socket&>())).WillRepeatedly(Invoke(
            [](auto& socket)
            {
                return std::make_unique<bzn::beast::websocket_stream>(std::move(socket));
            }));

        auto node = std::make_shared<bzn::node>(mock_io_context, mock_websocket, mock_chaos, std::chrono::milliseconds(0), TEST_ENDPOINT, crypto, options);
        node->start();

        // call the handler to test do_accept() is not called on error and do_accept() is called again...
        accept_handler(boost::asio::error::operation_aborted);

        // call with no error and a session should be created...
        accept_handler(boost::system::error_code());

        // calling start again should not call do_accept()...
        node->start();
    }


    TEST(node, test_that_registering_message_handler_can_only_be_done_once)
    {
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto mock_io_context = std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>();
        auto options = std::shared_ptr<bzn::options>();
        auto crypto = std::shared_ptr<bzn::crypto>();
        auto node = std::make_shared<bzn::node>(mock_io_context, nullptr, mock_chaos, std::chrono::milliseconds(0), TEST_ENDPOINT, crypto, options);

        // test that nulls are rejected...
        ASSERT_FALSE(node->register_for_message("asdf", nullptr));

        // test that non-null handler is added...
        ASSERT_TRUE(node->register_for_message("asdf", [](const auto&, auto){}));

        // test that we can't overwrite previous registration...
        ASSERT_FALSE(node->register_for_message("asdf", [](const auto&, auto){}));
    }


    TEST(node, test_that_registered_message_handler_is_invoked)
    {
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto mock_io_context = std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>();
        auto options = std::shared_ptr<bzn::options>();
        auto crypto = std::shared_ptr<bzn::crypto>();
        auto node = std::make_shared<bzn::node>(mock_io_context, nullptr, mock_chaos, std::chrono::milliseconds(0), TEST_ENDPOINT, crypto, options);

        // Add our test callback...
        bool callback_execute = false;
        std::string msg_type;
        ASSERT_TRUE(node->register_for_message("asdf", [&](const auto& msg, auto)
        {
            msg_type = msg["bzn-api"].asString();
            callback_execute = true;
        }));

        // test no handler found...
        auto mock_session = std::make_shared<bzn::Mocksession_base>();
        Json::Value msg;
        msg["bzn-api"] = "1234";
        EXPECT_CALL(*mock_session, close());
        node->priv_msg_handler(msg, mock_session);
        EXPECT_FALSE(callback_execute);
        EXPECT_EQ(msg_type, "");

        // test handler found...
        msg["bzn-api"] = "asdf";
        node->priv_msg_handler(msg, mock_session);
        EXPECT_TRUE(callback_execute);
        EXPECT_EQ(msg_type, "asdf");
    }

    TEST(node, test_that_wrongly_signed_messages_are_dropped)
    {
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto mock_io_context = std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>();
        auto options = std::make_shared<bzn::options>();
        options->get_mutable_simple_options().set(bzn::option_names::CRYPTO_ENABLED_INCOMING, "true");
        auto crypto = std::make_shared<bzn::crypto>(options);
        auto mock_session = std::make_shared<bzn::Mocksession_base>();
        auto node = std::make_shared<bzn::node>(mock_io_context, nullptr, mock_chaos, std::chrono::milliseconds(0), TEST_ENDPOINT, crypto, options);

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


    TEST(node, test_that_send_msg_connects_and_performs_handshake)
    {
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
        auto mock_websocket = std::make_shared<bzn::beast::Mockwebsocket_base>();
        auto mock_socket = std::make_unique<bzn::asio::Mocktcp_socket_base>();
        auto mock_websocket_stream = std::make_unique<bzn::beast::Mockwebsocket_stream_base>();
        auto options = std::shared_ptr<bzn::options>();
        auto crypto = std::shared_ptr<bzn::crypto>();

        // satisfy constructor...
        EXPECT_CALL(*mock_io_context, make_unique_tcp_acceptor(_));
        auto node = std::make_shared<bzn::node>(mock_io_context, mock_websocket, mock_chaos, std::chrono::milliseconds(0), TEST_ENDPOINT, crypto, options);

        // setup expectations for connect...
        bzn::asio::connect_handler connect_handler;
        EXPECT_CALL(*mock_socket, async_connect(TEST_ENDPOINT, _)).WillOnce(Invoke(
           [&](const auto& /*ep*/, auto handler)
           {
               connect_handler = handler;
           }));

        EXPECT_CALL(*mock_io_context, make_unique_tcp_socket()).WillOnce(Invoke(
            [&]()
            {
                static boost::asio::io_context io;
                static boost::asio::ip::tcp::socket socket(io);
                EXPECT_CALL(*mock_socket, get_tcp_socket()).WillRepeatedly(ReturnRef(socket));
                return std::move(mock_socket);
            }));

        // intercept the async handshake handler...
        bzn::beast::handshake_handler handshake_handler;
        EXPECT_CALL(*mock_websocket_stream, async_handshake(_,_,_)).WillOnce(Invoke(
           [&](const auto&, const auto& , auto handler)
           {
                handshake_handler = handler;
           }));

        EXPECT_CALL(*mock_websocket, make_unique_websocket_stream(_)).WillOnce(Invoke(
            [&](auto& /*socket*/)
            {
                return std::move(mock_websocket_stream);
            }));

        node->send_message_json(TEST_ENDPOINT, std::make_shared<bzn::json_message>("{}"));

        // call with no error to validate handshake...
        connect_handler(boost::system::error_code());

        // nothing should happen...
        connect_handler(boost::asio::error::operation_aborted);
    }


    // ./node_tests --gtest_also_run_disabled_tests --gtest_filter=node.DISABLED_test_node
    TEST(node, DISABLED_test_node)
    {
        auto mock_chaos = std::make_shared<NiceMock<bzn::mock_chaos_base>>();
        auto io_context = std::make_shared<bzn::asio::io_context>();
        auto websocket = std::make_shared<bzn::beast::websocket>();
        auto options = std::shared_ptr<bzn::options>();
        auto crypto = std::shared_ptr<bzn::crypto>();

        boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4::from_string("127.0.0.1"), 8080};

        auto node = std::make_shared<bzn::node>(io_context, websocket, mock_chaos, std::chrono::milliseconds(0), ep, crypto, options);

        node->register_for_message("crud",
            [](const bzn::json_message& msg, std::shared_ptr<bzn::session_base> session)
            {
                LOG(info) << '\n' << msg.toStyledString().substr(0, MAX_MESSAGE_SIZE) << "...";

                // echo back what the client sent...
                session->send_message(std::make_shared<bzn::json_message>(msg), true);
            });

        node->start();

        io_context->run();
    }

} // namespace bzn
