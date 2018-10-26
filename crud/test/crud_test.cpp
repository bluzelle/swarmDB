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
#include <algorithm>

using namespace ::testing;


TEST(crud, test_that_create_sends_proper_response)
{
    bzn::pbft::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

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
            ASSERT_EQ(resp.response_case(), database_response::RESPONSE_NOT_SET);
        }));

    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);
}


TEST(crud, test_that_read_sends_proper_response)
{
    bzn::pbft::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

    // null session nothing should happen...
    crud.handle_request(msg, nullptr);
}


TEST(crud, test_that_update_sends_proper_response)
{
    bzn::pbft::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);
}


TEST(crud, test_that_delete_sends_proper_response)
{
    bzn::pbft::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);
}


TEST(crud, test_that_has_sends_proper_response)
{
    bzn::pbft::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

    // null session nothing should happen...
    crud.handle_request(msg, nullptr);
}


TEST(crud, test_that_keys_sends_proper_response)
{
    bzn::pbft::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create()->set_key("key1");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false)).Times(2);
    crud.handle_request(msg, session);

    // add another...
    msg.mutable_create()->set_key("key2");
    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

    // null session nothing should happen...
    crud.handle_request(msg, nullptr);
}


TEST(crud, test_that_size_sends_proper_response)
{
    bzn::pbft::crud crud(std::make_shared<bzn::mem_storage>(), nullptr);

    database_msg msg;

    msg.mutable_header()->set_db_uuid("uuid");
    msg.mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_create()->set_key("key");
    msg.mutable_create()->set_value("value");

    // add key...
    auto session = std::make_shared<bzn::Mocksession_base>();

    EXPECT_CALL(*session, send_message(An<std::shared_ptr<std::string>>(), false));
    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

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

    crud.handle_request(msg, session);

    // null session nothing should happen...
    crud.handle_request(msg, nullptr);
}