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

#pragma once
#include <gtest/gtest.h>
#include <include/bluzelle.hpp>
#include <pbft/operations/pbft_operation.hpp>
#include <pbft/pbft_base.hpp>
#include <pbft/pbft.hpp>
#include <pbft/dummy_pbft_service.hpp>
#include <pbft/pbft_failure_detector.hpp>
#include <storage/mem_storage.hpp>
#include <bootstrap/bootstrap_peers.hpp>
#include <mocks/mock_node_base.hpp>
#include <proto/bluzelle.pb.h>
#include <json/json.h>
#include <boost/beast/core/detail/base64.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_pbft_failure_detector.hpp>
#include <mocks/mock_pbft_service_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_monitor.hpp>
#include <mocks/smart_mock_node.hpp>
#include <crypto/crypto.hpp>
#include <options/options.hpp>
#include <storage/mem_storage.hpp>

using namespace ::testing;

namespace bzn::test
{
    const bzn::uuid_t TEST_NODE_UUID{"uuid1"};
    const bzn::uuid_t SECOND_NODE_UUID{"uuid0"};

    const std::string TEST_NODE_ADDR("127.0.0.1");
    const uint16_t TEST_NODE_LISTEN_PORT(8084);
    const uint16_t TEST_NODE_HTTP_PORT(8884);

    const bzn::peers_list_t TEST_PEER_LIST{{  "127.0.0.1", 8081, 8881, "name1", "uuid0"}
                                           , {"127.0.0.1", 8082, 8882, "name2", "uuid2"}
                                           , {"127.0.0.1", 8083, 8883, "name3", "uuid3"}
                                           , {TEST_NODE_ADDR, TEST_NODE_LISTEN_PORT, TEST_NODE_HTTP_PORT, "name4", TEST_NODE_UUID}};

    const std::string TEST_NODE_ADDR_0("127.0.0.1");

    const bzn::peers_list_t BAD_TEST_PEER_LIST_0{{ "127.0.0.1", 8081, 8881, "name1", "uuid0"}
                                                , {"127.0.0.1", 8082, 8882, "name2", "uuid2"}
                                                , {"127.0.0.1", 8083, 8883, "name3", "uuid3"}
                                                , {TEST_NODE_ADDR_0, TEST_NODE_LISTEN_PORT, TEST_NODE_HTTP_PORT, "name4", "uuid_of_bad_node"}};

    const bzn::peers_list_t GOOD_TEST_PEER_LIST{{ "127.0.0.1", 8081, 8881, "name1", "uuid0"}
                                                , {"127.0.0.1", 8082, 8882, "name2", "uuid2"}
                                                , {"127.0.0.1", 8083, 8883, "name3", "uuid3"}
                                                , {TEST_NODE_ADDR_0, uint16_t(TEST_NODE_LISTEN_PORT + 73), TEST_NODE_HTTP_PORT, "name4", "uuid_of_good_node"}};

    class pbft_test : public Test
    {
    public:
        bzn_envelope request_msg;

        pbft_msg preprepare_msg;
        bzn_envelope default_original_msg;

        std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context =
                std::make_shared<NiceMock<bzn::asio::Mockio_context_base >>();
        std::shared_ptr<bzn::smart_mock_node> mock_node = std::make_shared<bzn::smart_mock_node>();
        std::shared_ptr<bzn::Mockpbft_failure_detector_base> mock_failure_detector =
                std::make_shared<NiceMock<bzn::Mockpbft_failure_detector_base >>();
        std::shared_ptr<bzn::mock_pbft_service_base> mock_service =
                std::make_shared<NiceMock<bzn::mock_pbft_service_base>>();
        std::shared_ptr<bzn::Mocksession_base> mock_session =
                std::make_shared<NiceMock<bzn::Mocksession_base>>();
        std::shared_ptr<bzn::storage_base> storage = std::make_shared<bzn::mem_storage>();
        std::shared_ptr<bzn::pbft_operation_manager> operation_manager =
                std::make_shared<bzn::pbft_operation_manager>(storage);

        std::shared_ptr<bzn::options_base> options = std::make_shared<bzn::options>();
        std::shared_ptr<bzn::mock_monitor> monitor = std::make_shared<NiceMock<bzn::mock_monitor>>();
        std::shared_ptr<bzn::crypto_base> crypto = std::make_shared<bzn::crypto>(options, monitor);

        std::shared_ptr<bzn::pbft> pbft;

        std::unique_ptr<bzn::asio::Mocksteady_timer_base> audit_heartbeat_timer =
                std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base >>();

        std::unique_ptr<bzn::asio::Mocksteady_timer_base> new_config_timer =
                std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base >>();

        std::unique_ptr<bzn::asio::Mocksteady_timer_base> join_retry_timer =
            std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base >>();

        std::unique_ptr<bzn::asio::Mocksteady_timer_base> cp_manager_timer1 =
                std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base >>();

        bzn::asio::wait_handler audit_heartbeat_timer_callback;
        bzn::asio::wait_handler new_config_timer_callback;

        size_t cp_manager_timer_callback_count = 0;
        std::unordered_map<size_t, bzn::asio::wait_handler> cp_manager_timer_callbacks;

        bzn::execute_handler_t service_execute_handler;
        bzn::protobuf_handler message_handler;
        bzn::protobuf_handler database_handler;
        bzn::protobuf_handler database_response_handler;
        bzn::protobuf_handler membership_handler;
        bzn::protobuf_handler checkpoint_msg_handler;

        bzn::uuid_t uuid = TEST_NODE_UUID;

        bool pbft_built = false;

        pbft_test();

        void build_pbft();

        void TearDown();

        void send_preprepare(uint64_t view, uint64_t sequence, bzn::hash_t req_hash, std::optional<bzn_envelope> request);
        void send_prepares(uint64_t view, uint64_t sequence, bzn::hash_t req_hash);
        void send_commits(uint64_t view, uint64_t sequence, bzn::hash_t req_hash);


    };

    uint64_t now();

    pbft_msg extract_pbft_msg(std::string msg);
    uuid_t extract_sender(std::string msg);

    bzn_envelope
    wrap_pbft_msg(const pbft_msg& msg, const bzn::uuid_t sender="");

    bzn_envelope wrap_pbft_membership_msg(const pbft_membership_msg& msg, const bzn::uuid_t sender);

    bzn_envelope
    wrap_request(const database_msg& msg);

    bool is_preprepare(std::shared_ptr<bzn_envelope> msg);
    bool is_prepare(std::shared_ptr<bzn_envelope> msg);
    bool is_commit(std::shared_ptr<bzn_envelope> msg);
    bool is_checkpoint(std::shared_ptr<bzn_envelope> msg);
    bool is_join(std::shared_ptr<bzn_envelope> msg);
    bool is_audit(std::shared_ptr<bzn_envelope> msg);
    bool is_viewchange(std::shared_ptr<bzn_envelope> wrapped_msg);
    bool is_newview(std::shared_ptr<bzn_envelope> wrapped_msg);

    bzn_envelope from(uuid_t uuid);
}
