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

#include <crud/crud.hpp>
#include <storage/mem_storage.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_subscription_manager_base.hpp>
#include <algorithm>

using namespace ::testing;


TEST(crud, test_that_create_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.error().message(), bzn::MSG_DATABASE_NOT_FOUND);
        }));

    crud.handle_request("caller_id", msg, session);

    // now create the db...
    msg.release_create();
    msg.mutable_create_db();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::RESPONSE_NOT_SET);
        }));

    crud.handle_request("caller_id", msg, session);

    // now test creates...
    msg.release_create_db();
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::RESPONSE_NOT_SET);
        }));

    crud.handle_request("caller_id", msg, session);

    // fail to create same key...
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kError);
            ASSERT_EQ(resp.error().message(), bzn::MSG_RECORD_EXISTS);
        }));

    crud.handle_request("caller_id", msg, session);

    // fail because key is too big...
    msg.mutable_create()->set_key(std::string(bzn::MAX_KEY_SIZE+1,'*'));
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kError);
            ASSERT_EQ(resp.error().message(), bzn::MSG_KEY_SIZE_TOO_LARGE);
        }));

    crud.handle_request("caller_id", msg, session);

    // fail because value is too big...
    msg.mutable_create()->set_value(std::string(bzn::MAX_VALUE_SIZE+1,'*'));
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kError);
            ASSERT_EQ(resp.error().message(), bzn::MSG_VALUE_SIZE_TOO_LARGE);
        }));

    crud.handle_request("caller_id", msg, session);
}


TEST(crud, test_that_read_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create_db();

    crud.handle_request("caller_id", msg, nullptr);

    // test reads...
    msg.release_create_db();
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request("caller_id", msg, session);

    // clear msg...
    msg.mutable_create()->release_key();
    msg.mutable_create()->release_value();

    // read key...
    msg.mutable_read()->set_key("key");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kRead);
            ASSERT_EQ(resp.read().key(), "key");
            ASSERT_EQ(resp.read().value(), "value");
        }));

    crud.handle_request("caller_id", msg, session);

    // read invalid key...
    msg.mutable_read()->set_key("invalid-key");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kError);
            ASSERT_EQ(resp.error().message(), bzn::MSG_RECORD_NOT_FOUND);
        }));

    crud.handle_request("caller_id", msg, session);

    // null session nothing should happen...
    crud.handle_request("caller_id", msg, nullptr);
}


TEST(crud, test_that_update_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create_db();

    crud.handle_request("caller_id", msg, nullptr);

    // test updates...
    msg.release_create_db();
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request("caller_id", msg, session);

    // clear msg...
    msg.mutable_create()->release_key();
    msg.mutable_create()->release_value();

    // update key...
    msg.mutable_update()->set_key("key");
    msg.mutable_update()->set_value("updated");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::RESPONSE_NOT_SET);
        }));

    crud.handle_request("caller_id", msg, session);

    // clear msg...
    msg.mutable_update()->release_key();
    msg.mutable_update()->release_value();

    // read updated key...
    msg.mutable_read()->set_key("key");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kRead);
            ASSERT_EQ(resp.read().key(), "key");
            ASSERT_EQ(resp.read().value(), "updated");
        }));

    crud.handle_request("caller_id", msg, session);
}


TEST(crud, test_that_delete_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create_db();

    crud.handle_request("caller_id", msg, nullptr);

    // test deletes...
    msg.release_create_db();
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request("caller_id", msg, session);

    // clear msg...
    msg.mutable_create()->release_key();
    msg.mutable_create()->release_value();

    // delete key...
    msg.mutable_delete_()->set_key("key");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::RESPONSE_NOT_SET);
        }));

    crud.handle_request("caller_id", msg, session);

    // delete invalid key...
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kError);
            ASSERT_EQ(resp.error().message(), bzn::MSG_RECORD_NOT_FOUND);
        }));

    crud.handle_request("caller_id", msg, session);
}


TEST(crud, test_that_has_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create_db();

    crud.handle_request("caller_id", msg, nullptr);

    // test has...
    msg.release_create_db();
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request("caller_id", msg, session);

    // clear msg...
    msg.mutable_create()->release_key();
    msg.mutable_create()->release_value();

    // valid key...
    msg.mutable_has()->set_key("key");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::RESPONSE_NOT_SET);
        }));

    crud.handle_request("caller_id", msg, session);

    // invalid key...
    msg.mutable_has()->set_key("invalid-key");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kError);
            ASSERT_EQ(resp.error().message(), bzn::MSG_RECORD_NOT_FOUND);
        }));

    crud.handle_request("caller_id", msg, session);

    // null session nothing should happen...
    crud.handle_request("caller_id", msg, nullptr);
}


TEST(crud, test_that_keys_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create_db();

    crud.handle_request("caller_id", msg, nullptr);

    // test keys...
    msg.release_create_db();
    msg.mutable_create()->set_key("key1");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).Times(2);
    crud.handle_request("caller_id", msg, session);

    // add another...
    msg.mutable_create()->set_key("key2");
    crud.handle_request("caller_id", msg, session);

    // clear msg...
    msg.mutable_create()->release_key();
    msg.mutable_create()->release_value();

    // get keys...
    msg.mutable_keys();
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kKeys);
            ASSERT_EQ(resp.keys().keys().size(), int(2));
            // keys are not returned in order created...
            auto keys = resp.keys().keys();
            std::sort(keys.begin(), keys.end());
            ASSERT_EQ(keys[0], "key1");
            ASSERT_EQ(keys[1], "key2");
        }));

    crud.handle_request("caller_id", msg, session);

    // invalid uuid returns empty message...
    msg.mutable_header()->set_db_uuid("invalid-uuid");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "invalid-uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kKeys);
            ASSERT_EQ(resp.keys().keys().size(), int(0));
        }));

    crud.handle_request("caller_id", msg, session);

    // null session nothing should happen...
    crud.handle_request("caller_id", msg, nullptr);
}


TEST(crud, test_that_size_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create_db();

    crud.handle_request("caller_id", msg, nullptr);

    // test size...
    msg.release_create_db();
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request("caller_id", msg, session);

    // clear msg...
    msg.mutable_create()->release_key();
    msg.mutable_create()->release_value();

    // get size...
    msg.mutable_size();
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kSize);
            ASSERT_EQ(resp.size().bytes(), int32_t(5));
            ASSERT_EQ(resp.size().keys(), int32_t(1));
        }));

    crud.handle_request("caller_id", msg, session);

    // invalid uuid returns zero...
    msg.mutable_header()->set_db_uuid("invalid-uuid");
    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [&](auto msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "invalid-uuid");
            ASSERT_EQ(resp.header().transaction_id(), uint64_t(123));
            ASSERT_EQ(resp.response_case(), database_response::kSize);
            ASSERT_EQ(resp.size().bytes(), int32_t(0));
            ASSERT_EQ(resp.size().keys(), int32_t(0));
        }));

    crud.handle_request("caller_id", msg, session);

    // null session nothing should happen...
    crud.handle_request("caller_id", msg, nullptr);
}


TEST(crud, test_that_subscribe_request_calls_subscription_manager)
{
    auto mock_subscription_manager = std::make_shared<bzn::Mocksubscription_manager_base>();

    bzn::crud crud(std::make_shared<bzn::mem_storage>(), mock_subscription_manager);

    EXPECT_CALL(*mock_subscription_manager, start());

    crud.start();

    // subscribe...
    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_subscribe()->set_key("key");

    // nothing should happen...
    crud.handle_request("caller_id", msg, nullptr);

    // try again with a valid session...
    auto mock_session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*mock_subscription_manager, subscribe(msg.header().db_uuid(), msg.subscribe().key(),
        msg.header().transaction_id(), _, _));

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false));

    crud.handle_request("caller_id", msg, mock_session);
}


TEST(crud, test_that_unsubscribe_request_calls_subscription_manager)
{
    auto mock_subscription_manager = std::make_shared<bzn::Mocksubscription_manager_base>();

    bzn::crud crud(std::make_shared<bzn::mem_storage>(), mock_subscription_manager);

    EXPECT_CALL(*mock_subscription_manager, start());

    crud.start();

    // unsubscribe...
    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_unsubscribe()->set_key("key");
    msg.mutable_unsubscribe()->set_transaction_id(321);

    // nothing should happen...
    crud.handle_request("caller_id", msg, nullptr);
    
    auto mock_session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*mock_subscription_manager, unsubscribe(msg.header().db_uuid(), msg.unsubscribe().key(),
        msg.unsubscribe().transaction_id(), _, _));

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false));

    crud.handle_request("caller_id", msg, mock_session);
}


TEST(crud, test_that_has_db_request_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), std::make_shared<NiceMock<bzn::Mocksubscription_manager_base>>());

    crud.start();

    // has db...
    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_has_db();

    // nothing should happen...
    crud.handle_request("caller_id", msg, nullptr);

    auto mock_session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [](std::shared_ptr<bzn::encoded_message> msg, bool /*end_session*/)
        {
            database_response resp;

            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.error().message(), bzn::MSG_RECORD_NOT_FOUND);
        }));

    crud.handle_request("caller_id", msg, mock_session);
}


TEST(crud, test_that_create_db_request_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), std::make_shared<NiceMock<bzn::Mocksubscription_manager_base>>());

    crud.start();

    // create database...
    database_msg msg;
    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create_db();

    auto mock_session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [](std::shared_ptr<bzn::encoded_message> msg, bool /*end_session*/)
        {
            database_response resp;

            EXPECT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.response_case(), database_response::RESPONSE_NOT_SET);
        }));

    crud.handle_request("caller_id", msg, mock_session);

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [](std::shared_ptr<bzn::encoded_message> msg, bool /*end_session*/)
        {
            database_response resp;

            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.error().message(), bzn::MSG_RECORD_EXISTS);
        }));

    // try to create it again...
    crud.handle_request("caller_id", msg, mock_session);
}


TEST(crud, test_that_delete_db_sends_proper_response)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), std::make_shared<NiceMock<bzn::Mocksubscription_manager_base>>());

    crud.start();

    // delete database...
    database_msg msg;
    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_delete_db();

    auto mock_session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [](std::shared_ptr<bzn::encoded_message> msg, bool /*end_session*/)
        {
            database_response resp;

            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.error().message(), bzn::MSG_RECORD_NOT_FOUND);
        }));

    crud.handle_request("caller_id", msg, mock_session);

    // create a database...
    msg.release_delete_db();
    msg.mutable_create_db();

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false));

    crud.handle_request("caller_id", msg, mock_session);

    // delete database...
    msg.release_create_db();
    msg.mutable_delete_db();

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [](std::shared_ptr<bzn::encoded_message> msg, bool /*end_session*/)
        {
            database_response resp;

            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.error().message(), bzn::MSG_ACCESS_DENIED);
        }));

    // non-owner caller...
    crud.handle_request("bad_caller_id", msg, mock_session);

    EXPECT_CALL(*mock_session, send_message(An<std::shared_ptr<std::string>>(), false)).WillOnce(Invoke(
        [](std::shared_ptr<bzn::encoded_message> msg, bool /*end_session*/)
        {
            database_response resp;

            ASSERT_TRUE(resp.ParseFromString(*msg));
            ASSERT_EQ(resp.header().db_uuid(), "uuid");
            ASSERT_EQ(resp.response_case(), database_response::RESPONSE_NOT_SET);
        }));

    crud.handle_request("caller_id", msg, mock_session);
}


TEST(crud, test_that_state_can_be_saved_and_retrieved)
{
    bzn::crud crud(std::make_shared<bzn::mem_storage>(), std::make_shared<NiceMock<bzn::Mocksubscription_manager_base>>());

    ASSERT_TRUE(crud.save_state());

    auto state = crud.get_saved_state();

    ASSERT_TRUE(state);

    ASSERT_TRUE(crud.load_state(*state));
}