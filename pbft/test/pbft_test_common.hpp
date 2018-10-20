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
#include <pbft/pbft_operation.hpp>
#include <pbft/pbft_base.hpp>
#include <pbft/pbft.hpp>
#include <pbft/dummy_pbft_service.hpp>
#include <pbft/pbft_failure_detector.hpp>
#include <bootstrap/bootstrap_peers.hpp>
#include <mocks/mock_node_base.hpp>
#include <proto/bluzelle.pb.h>
#include <json/json.h>
#include <boost/beast/core/detail/base64.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_pbft_failure_detector.hpp>
#include <mocks/mock_pbft_service_base.hpp>

using namespace ::testing;

namespace bzn::test
{
    const bzn::uuid_t TEST_NODE_UUID{"uuid1"};
    const bzn::uuid_t SECOND_NODE_UUID{"uuid0"};

    const bzn::peers_list_t TEST_PEER_LIST{{  "127.0.0.1", 8081, 8881, "name1", "uuid0"}
                                           , {"127.0.0.1", 8082, 8882, "name2", "uuid2"}
                                           , {"127.0.0.1", 8083, 8883, "name3", "uuid3"}
                                           , {"127.0.0.1", 8084, 8884, "name4", TEST_NODE_UUID}};


    class pbft_test : public Test
    {
    public:
        pbft_msg request_msg;
        pbft_msg preprepare_msg;
        wrapped_bzn_msg default_original_msg;

        std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context =
                std::make_shared<NiceMock<bzn::asio::Mockio_context_base >>();
        std::shared_ptr<bzn::Mocknode_base> mock_node = std::make_shared<NiceMock<bzn::Mocknode_base >>();
        std::shared_ptr<bzn::pbft_failure_detector_base> mock_failure_detector =
                std::make_shared<NiceMock<bzn::Mockpbft_failure_detector_base >>();
        std::shared_ptr<bzn::mock_pbft_service_base> mock_service =
                std::make_shared<NiceMock<bzn::mock_pbft_service_base>>();

        std::shared_ptr<bzn::pbft> pbft;

        std::unique_ptr<bzn::asio::Mocksteady_timer_base> audit_heartbeat_timer =
                std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base >>();

        bzn::asio::wait_handler audit_heartbeat_timer_callback;

        std::function<void(const pbft_request&, uint64_t)> service_execute_handler;
        bzn::protobuf_handler message_handler;
        bzn::message_handler database_handler;

        bzn::uuid_t uuid = TEST_NODE_UUID;

        bool pbft_built = false;

        pbft_test();

        void build_pbft();

        void TearDown();
    };


    pbft_msg extract_pbft_msg(std::string msg);
    uuid_t extract_sender(std::string msg);
    wrapped_bzn_msg
    wrap_pbft_msg(const pbft_msg& msg);
    bool is_preprepare(std::shared_ptr<std::string> msg);
    bool is_prepare(std::shared_ptr<std::string> msg);
    bool is_commit(std::shared_ptr<std::string> msg);
    bool is_checkpoint(std::shared_ptr<std::string> msg);
    bool is_audit(std::shared_ptr<std::string> msg);

    wrapped_bzn_msg from(uuid_t uuid);
}
