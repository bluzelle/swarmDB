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

#include <mocks/mock_node_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <crud/subscription_manager.hpp>

using namespace ::testing;

namespace
{
    const bzn::key_t  TEST_KEY{"key"};
    const bzn::uuid_t TEST_UUID{"uuid"};
    const bzn::uuid_t TEST_UNKOWN_UUID{"67ee0ca8-bd79-4eef-88db-9343aaf6ca7e"};
}


TEST(subscription_manager, test_that_session_can_subscribe_and_unsubscribe_for_updates)
{
    auto mock_session1 = std::make_shared<bzn::Mocksession_base>();
    auto mock_session2 = std::make_shared<bzn::Mocksession_base>();

    bzn::subscription_manager sm(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>());

    // two subscribers...
    {
        database_response response;
        sm.subscribe(TEST_UUID, TEST_KEY, 0, response, mock_session1);

        ASSERT_EQ(response.response_case(), database_response::RESPONSE_NOT_SET);
    }

    {
        database_response response;
        sm.subscribe(TEST_UUID, TEST_KEY, 1, response, mock_session2);

        ASSERT_EQ(response.response_case(), database_response::RESPONSE_NOT_SET);
    }

    // duplicate subscription request...
    {
        database_response response;
        sm.subscribe(TEST_UUID, TEST_KEY, 2, response, mock_session1);

        ASSERT_EQ(response.response_case(), database_response::kError);
        ASSERT_EQ(response.error().message(), bzn::MSG_DUPLICATE_SUB);
    }

    // succeed and we should be return a success response...
    {
        database_response response;
        sm.unsubscribe(TEST_UUID, TEST_KEY, 3, response, mock_session1);

        ASSERT_EQ(response.response_case(), database_response::RESPONSE_NOT_SET);
    }

    // should get an unknown subscription error..
    {
        database_response response;
        sm.unsubscribe(TEST_UUID, TEST_KEY, 4, response, mock_session1);

        ASSERT_EQ(response.response_case(), database_response::kError);
        ASSERT_EQ(response.error().message(), bzn::MSG_INVALID_SUB);
    }

    // test for invalid uuid...
    {
        database_response response;
        sm.unsubscribe(TEST_UNKOWN_UUID, TEST_KEY, 5, response, mock_session2);

        ASSERT_EQ(response.response_case(), database_response::kError);
        ASSERT_EQ(response.error().message(), bzn::MSG_INVALID_UUID);
    }
}


TEST(subscription_manager, test_that_session_cannot_unsubscribe_other_sessions)
{
    auto mock_session1 = std::make_shared<bzn::Mocksession_base>();
    auto mock_session2 = std::make_shared<bzn::Mocksession_base>();

    bzn::subscription_manager sm(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>());

    // two subscribers...
    {
        database_response response;
        sm.subscribe(TEST_UUID, "1", 0, response, mock_session1);
        sm.subscribe(TEST_UUID, "2", 1, response, mock_session2);
    }

    // test for invalid uuid...
    database_response response;

    // session two will try to unsub session 1...
    sm.unsubscribe(TEST_UUID, "1", 2, response, mock_session2);

    ASSERT_EQ(response.error().message(), bzn::MSG_INVALID_SUB);
}


TEST(subscription_manager, test_that_subscriber_is_notified_for_create_and_updates)
{
    auto mock_session1 = std::make_shared<bzn::Mocksession_base>();
    auto mock_session2 = std::make_shared<bzn::Mocksession_base>();

    bzn::subscription_manager sm(std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>());

    database_response response;
    sm.subscribe(TEST_UUID, "0", 0, response, mock_session1);
    sm.subscribe(TEST_UUID, "0", 1234, response, mock_session2);
    sm.subscribe(TEST_UUID, "1", 4321, response, mock_session1);
    sm.subscribe(TEST_UUID, "1", 1, response, mock_session2);

    // send through a 'create' message...
    {
        database_msg msg;

        msg.mutable_header()->set_db_uuid(TEST_UUID);
        msg.mutable_header()->set_transaction_id(123);
        msg.mutable_create()->set_key("0");
        msg.mutable_create()->set_value("0");

        EXPECT_CALL(*mock_session1, send_datagram(An<std::shared_ptr<std::string>>())).WillOnce(Invoke(
            [](std::shared_ptr<std::string> msg)
            {
                database_response resp;
                resp.ParseFromString(*msg);
                ASSERT_EQ(resp.header().db_uuid(), TEST_UUID);
                ASSERT_EQ(resp.header().transaction_id(), uint64_t(0));
                ASSERT_EQ(resp.response_case(), database_response::kSubscriptionUpdate);
                ASSERT_EQ(resp.subscription_update().key(), "0");
                ASSERT_EQ(resp.subscription_update().value(), "0");
            }));

        EXPECT_CALL(*mock_session2, send_datagram(An<std::shared_ptr<std::string>>())).WillOnce(Invoke(
            [](std::shared_ptr<std::string> msg)
            {
                database_response resp;
                resp.ParseFromString(*msg);
                ASSERT_EQ(resp.header().db_uuid(), TEST_UUID);
                ASSERT_EQ(resp.header().transaction_id(), uint64_t(1234));
                ASSERT_EQ(resp.response_case(), database_response::kSubscriptionUpdate);
                ASSERT_EQ(resp.subscription_update().key(), "0");
                ASSERT_EQ(resp.subscription_update().value(), "0");
            }));

        sm.inspect_commit(msg);
    }

    // send through an 'update' message...
    {
        database_msg msg;

        msg.mutable_header()->set_db_uuid(TEST_UUID);
        msg.mutable_header()->set_transaction_id(123);
        msg.mutable_update()->set_key("1");
        msg.mutable_update()->set_value("1");

        EXPECT_CALL(*mock_session1, send_datagram(An<std::shared_ptr<std::string>>())).WillOnce(Invoke(
            [](std::shared_ptr<std::string> msg)
            {
                database_response resp;
                resp.ParseFromString(*msg);
                ASSERT_EQ(resp.header().db_uuid(), TEST_UUID);
                ASSERT_EQ(resp.header().transaction_id(), uint64_t(4321));
                ASSERT_EQ(resp.response_case(), database_response::kSubscriptionUpdate);
                ASSERT_EQ(resp.subscription_update().key(), "1");
                ASSERT_EQ(resp.subscription_update().value(), "1");
            }));

        EXPECT_CALL(*mock_session2, send_datagram(An<std::shared_ptr<std::string>>())).WillOnce(Invoke(
            [](std::shared_ptr<std::string> msg)
            {
                database_response resp;
                resp.ParseFromString(*msg);
                ASSERT_EQ(resp.header().db_uuid(), TEST_UUID);
                ASSERT_EQ(resp.header().transaction_id(), uint64_t(1));
                ASSERT_EQ(resp.response_case(), database_response::kSubscriptionUpdate);
                ASSERT_EQ(resp.subscription_update().key(), "1");
                ASSERT_EQ(resp.subscription_update().value(), "1");
            }));

        sm.inspect_commit(msg);
    }

    // send a delete... nothing should happen...
    {
        database_msg msg;

        msg.mutable_header()->set_db_uuid(TEST_UUID);
        msg.mutable_header()->set_transaction_id(123);
        msg.mutable_delete_()->set_key("1");

        sm.inspect_commit(msg);
    }
}


TEST(subscription_manager, test_that_dead_session_is_removed_from_subscriber_list)
{
    auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
    auto mock_steady_timer = std::make_unique<bzn::asio::Mocksteady_timer_base>();

    EXPECT_CALL(*mock_steady_timer, expires_from_now(_)).Times(2);

    bzn::asio::wait_handler wh;
    EXPECT_CALL(*mock_steady_timer, async_wait(_)).WillRepeatedly(Invoke(
        [&](auto handler)
        {
            wh = handler;
        }));

    EXPECT_CALL(*mock_io_context, make_unique_steady_timer()).WillOnce(Invoke(
        [&]()
        {
            return std::move(mock_steady_timer);
        }));

    auto sm = std::make_shared<bzn::subscription_manager>(mock_io_context);

    sm->start();

    auto mock_session1 = std::make_shared<bzn::Mocksession_base>();
    auto mock_session2 = std::make_shared<bzn::Mocksession_base>();

    // update message...
    database_msg msg;

    msg.mutable_header()->set_db_uuid(TEST_UUID);
    msg.mutable_header()->set_transaction_id(0);
    msg.mutable_update()->set_key("0");
    msg.mutable_update()->set_value("0");

    // two subs for the same key...
    database_response response;
    sm->subscribe(TEST_UUID, "0", 0, response, mock_session1);
    sm->subscribe(TEST_UUID, "0", 1, response, mock_session2);

    EXPECT_CALL(*mock_session1, send_datagram(An<std::shared_ptr<std::string>>())).WillOnce(Invoke(
        [](std::shared_ptr<std::string> msg)
        {
            database_response resp;
            resp.ParseFromString(*msg);
            ASSERT_EQ(resp.header().db_uuid(), TEST_UUID);
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(0));
            ASSERT_EQ(resp.response_case(), database_response::kSubscriptionUpdate);
            ASSERT_EQ(resp.subscription_update().key(), "0");
            ASSERT_EQ(resp.subscription_update().value(), "0");
        }));

    EXPECT_CALL(*mock_session2, send_datagram(An<std::shared_ptr<std::string>>())).WillOnce(Invoke(
        [](std::shared_ptr<std::string> msg)
        {
            database_response resp;
            resp.ParseFromString(*msg);
            ASSERT_EQ(resp.header().db_uuid(), TEST_UUID);
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(1));
            ASSERT_EQ(resp.response_case(), database_response::kSubscriptionUpdate);
            ASSERT_EQ(resp.subscription_update().key(), "0");
            ASSERT_EQ(resp.subscription_update().value(), "0");
        }));

    sm->inspect_commit(msg);

    // kill session...
    mock_session2.reset();

    // exceute purge...
    wh(boost::system::error_code());

    // there should only be one...
    EXPECT_CALL(*mock_session1, send_datagram(An<std::shared_ptr<std::string>>())).WillOnce(Invoke(
        [](std::shared_ptr<std::string> msg)
        {
            database_response resp;
            resp.ParseFromString(*msg);
            ASSERT_EQ(resp.header().db_uuid(), TEST_UUID);
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(0));
            ASSERT_EQ(resp.response_case(), database_response::kSubscriptionUpdate);
            ASSERT_EQ(resp.subscription_update().key(), "0");
            ASSERT_EQ(resp.subscription_update().value(), "0");
        }));

    sm->inspect_commit(msg);
}
