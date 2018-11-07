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

#include <status/status.hpp>
#include <proto/status.pb.h>
#include <swarm_git_commit.hpp>
#include <mocks/mock_status_provider_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_node_base.hpp>

using namespace bzn;
using namespace testing;


TEST(status_test, test_that_status_registers_and_responses_to_requests)
{
    auto mock_node = std::make_shared<Mocknode_base>();

    // success
    {
        auto status = std::make_shared<bzn::status>(mock_node, bzn::status::status_provider_list_t{}, true);

        EXPECT_CALL(*mock_node, register_for_message("status", _)).WillOnce(Return(true));
        EXPECT_CALL(*mock_node, register_for_message(bzn_envelope::kStatusRequest, _)).WillOnce(Return(true));

        status->start();
    }

    // failure
    {
        auto status = std::make_shared<bzn::status>(mock_node, bzn::status::status_provider_list_t{}, true);

        EXPECT_CALL(*mock_node, register_for_message("status", _)).WillOnce(Return(false));

        EXPECT_THROW(status->start(), std::runtime_error);
    }

    // failure
    {
        auto status = std::make_shared<bzn::status>(mock_node, bzn::status::status_provider_list_t{}, false);

        EXPECT_CALL(*mock_node, register_for_message("status", _)).WillOnce(Return(true));
        EXPECT_CALL(*mock_node, register_for_message(bzn_envelope::kStatusRequest, _)).WillOnce(Return(false));

        EXPECT_THROW(status->start(), std::runtime_error);
    }
}


TEST(status_test, test_that_status_request_queries_status_providers)
{
    auto mock_node = std::make_shared<Mocknode_base>();
    auto mock_session = std::make_shared<Mocksession_base>();

    auto mock_status_provider = std::make_shared<Mockstatus_provider_base>();

    auto status = std::make_shared<bzn::status>(mock_node, bzn::status::status_provider_list_t{mock_status_provider, mock_status_provider}, true);

    bzn::message_handler mh;
    EXPECT_CALL(*mock_node, register_for_message("status", _)).WillOnce(Invoke(
        [&](const std::string&, auto handler)
        {
            mh = handler;
            return true;
        }));

    bzn::protobuf_handler pbh;
    EXPECT_CALL(*mock_node, register_for_message(bzn_envelope::kStatusRequest, _)).WillOnce(Invoke(
        [&](auto, auto handler)
        {
            pbh = handler;
            return true;
        }));

    status->start();

    // make a request...
    bzn::json_message request;
    request["bzn-api"] = "status";
    request["cmd"] = "state";
    request["transaction-id"] = 85746;

    EXPECT_CALL(*mock_status_provider, get_status()).WillRepeatedly(Invoke(
        []()
        {
            bzn::json_message status;
            status["line"] = __LINE__;
            status["file"] = __FILE__;
            return status;
        }));

    EXPECT_CALL(*mock_status_provider, get_name()).WillOnce(Invoke(
        [](){ return "mock1";})).WillOnce(Invoke([](){return "mock2";}));

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<bzn::json_message>>(), false)).WillOnce(Invoke(
        [&](std::shared_ptr<bzn::json_message> msg, bool)
        {
            ASSERT_EQ((*msg)["transaction-id"].asInt64(), request["transaction-id"].asInt64());
            ASSERT_EQ((*msg)["pbft_enabled"].asBool(), true);
            ASSERT_EQ((*msg)["module"].size(), size_t(2));
            ASSERT_EQ((*msg)["module"][0]["name"].asString(), "mock1");
            ASSERT_EQ((*msg)["module"][1]["name"].asString(), "mock2");
        }));

    mh(request, mock_session);

    // make protobuf request...
    EXPECT_CALL(*mock_status_provider, get_name()).WillOnce(Invoke(
        [](){ return "mock1";})).WillOnce(Invoke([](){return "mock2";}));

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<bzn::encoded_message>>(), false)).WillOnce(Invoke(
        [&](std::shared_ptr<bzn::encoded_message> msg, bool)
        {
            status_response sr;

            ASSERT_TRUE(sr.ParseFromString(*msg));
            ASSERT_TRUE(sr.pbft_enabled());
            ASSERT_EQ(sr.swarm_version(), SWARM_VERSION);
            ASSERT_EQ(sr.swarm_git_commit(), SWARM_GIT_COMMIT);
            ASSERT_EQ(sr.uptime(), "0 days, 0 hours, 0 minutes");

            Json::Value ms;
            Json::Reader reader;
            reader.parse(sr.module_status_json(), ms);
            ASSERT_EQ(ms["module"].size(), size_t(2));
            ASSERT_EQ(ms["module"][0]["name"].asString(), "mock1");
            ASSERT_EQ(ms["module"][1]["name"].asString(), "mock2");
        }));

    pbh(bzn_envelope(), mock_session);
}
