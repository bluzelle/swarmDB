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

#include <bootstrap/bootstrap_peers.hpp>

#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_node_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <mocks/mock_raft_base.hpp>
#include <mocks/mock_storage_base.hpp>

using namespace ::testing;


namespace
{
    const bzn::uuid_t TEST_NODE_UUID{"f0645cc2-476b-485d-b589-217be3ca87d5"};
    const bzn::uuid_t LEADER_UUID{"7110d050-42b3-11e8-842f-0ed5f89f718b"};
    const bzn::uuid_t USER_UUID{"80174b53-2dda-49f1-9d6a-6a780d4cceca"};
    const std::string TEST_VALUE = "I2luY2x1ZGUgPG1vY2tzL21vY2tfbm9kZV9iYXNlLmhwcD4NCiNpbmNsdWRlIDxtb2Nrcy9tb2NrX3Nlc3Npb25fYmFzZS5ocHA+DQojaW5jbHVkZSA8bW9ja3MvbW9ja19yYWZ0X2Jhc2UuaHBwPg0KI2luY2x1ZGUgPG1vY2tzL21vY2tfc3RvcmFnZV9iYXNlLmhwcD4NCg==";

    bzn::message generate_generic_request(const bzn::uuid_t& uid, bzn_msg& msg)
    {
        msg.mutable_db()->mutable_header()->set_db_uuid(uid);
        msg.mutable_db()->mutable_header()->set_transaction_id(85746);

        bzn::message request;

        request["bzn-api"] = "database";
        request["msg"] = boost::beast::detail::base64_encode(msg.SerializeAsString());

        return request;
    }

    bzn::message generate_create_request(const bzn::uuid_t& uid, const std::string& key, const std::string& value)
    {
        bzn_msg msg;

        msg.mutable_db()->mutable_create()->set_key(key);
        msg.mutable_db()->mutable_create()->set_value(value);

        return generate_generic_request(uid, msg);
    }

    bzn::message generate_read_request(const bzn::uuid_t& uid, const std::string& key)
    {
        bzn_msg msg;

        msg.mutable_db()->mutable_read()->set_key(key);

        return generate_generic_request(uid, msg);
    }

    bzn::message generate_update_request(const bzn::uuid_t& uid, const std::string& key, const std::string& value)
    {
        bzn_msg msg;

        msg.mutable_db()->mutable_update()->set_key(key);
        msg.mutable_db()->mutable_update()->set_value(value);

        return generate_generic_request(uid, msg);
    }

    bzn::message generate_delete_request(const bzn::uuid_t& uid, const std::string& key)
    {
        bzn_msg msg;

        msg.mutable_db()->mutable_delete_()->set_key(key);

        return generate_generic_request(uid, msg);
    }

    bzn::message generate_keys_request(const bzn::uuid_t& uid)
    {
        bzn_msg msg;

        msg.mutable_db()->mutable_keys();

        return generate_generic_request(uid, msg);
    }

    bzn::message generate_has_request(const bzn::uuid_t& uid, const std::string& key)
    {
        bzn_msg msg;

        msg.mutable_db()->mutable_has()->set_key(key);

        return generate_generic_request(uid, msg);
    }

    bzn::message generate_size_request(const bzn::uuid_t& uid)
    {
        bzn_msg msg;

        msg.mutable_db()->mutable_size();

        return generate_generic_request(uid, msg);
    }
}


class crud_test : public Test
{
public:
    crud_test()
    {
        this->mock_node = std::make_shared<bzn::Mocknode_base>();
        this->mock_raft = std::make_shared<bzn::Mockraft_base>();
        this->mock_storage = std::make_shared<bzn::Mockstorage_base>();
        this->mock_session = std::make_shared<bzn::Mocksession_base>();
    }

    void SetUp() final
    {
        EXPECT_CALL(*this->mock_node, register_for_message("database", _)).WillOnce(Invoke(
            [&](const std::string&, auto  mh)
            {
                this->mh = mh;
                return true;
            }));

        EXPECT_CALL(*mock_raft, register_commit_handler(_)).WillOnce(Invoke(
            [&](bzn::raft_base::commit_handler ch) { this->ch = ch; }));

        this->crud = std::make_shared<bzn::crud>(mock_node, mock_raft, mock_storage);

        this->crud->start();
    }

    std::shared_ptr<bzn::Mocknode_base> mock_node;
    std::shared_ptr<bzn::Mockraft_base> mock_raft;
    std::shared_ptr<bzn::Mockstorage_base> mock_storage;
    std::shared_ptr<bzn::Mocksession_base> mock_session;

    bzn::message_handler mh;
    bzn::raft_base::commit_handler ch;
    std::shared_ptr<bzn::crud> crud;
};


TEST_F(crud_test, test_that_follower_not_knowing_leader_fails_to_create)
{
    auto request = generate_create_request(USER_UUID, "key0", TEST_VALUE);

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    // We don't know the leader, maybe this node just started.
    EXPECT_CALL(*this->mock_raft, get_leader()).WillOnce(Return(bzn::peer_address_t("",0,0,"","")));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
           database_response resp;
           ASSERT_TRUE(resp.ParseFromString(*msg));
           EXPECT_EQ(resp.header().transaction_id(), uint64_t(85746));
           EXPECT_EQ(resp.header().db_uuid(), USER_UUID);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_follower_knowing_leader_fails_to_create)
{
    // must respond with error, and respond with leader uuid
    auto msg = generate_create_request(USER_UUID, "key0", TEST_VALUE);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_raft, get_leader()).WillOnce(Return(bzn::peer_address_t("127.0.0.1",49153,8080,"iron maiden",LEADER_UUID)));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kRedirect);
            EXPECT_EQ(resp.redirect().leader_id(), LEADER_UUID);
            EXPECT_EQ(resp.redirect().leader_host(), "127.0.0.1");
            EXPECT_EQ(resp.redirect().leader_port(), uint32_t(49153));
            EXPECT_EQ(resp.redirect().leader_name(), "iron maiden");
            EXPECT_EQ(resp.redirect().leader_http_port(), uint32_t(8080));
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_candidate_fails_to_create)
{
    // must respond with error
    auto msg = generate_create_request(USER_UUID, "key0", TEST_VALUE);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::candidate));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kResp);
            EXPECT_EQ(resp.resp().error(), bzn::MSG_ELECTION_IN_PROGRESS);
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_create_a_new_record)
{
    // must confirm with raft and create the record in storage
    // must respond with error
    auto request = generate_create_request(USER_UUID, "key0", "skdif9ek34587fk30df6vm73==");

    EXPECT_CALL(*this->mock_raft, get_state()).WillRepeatedly(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_raft, append_log(_)).WillOnce(Return(true));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));

        }));

    EXPECT_CALL(*this->mock_storage, has(USER_UUID, "key0")).WillOnce(Return(false));

    this->mh(request, this->mock_session);

    EXPECT_CALL(*this->mock_storage, create(USER_UUID, "key0", "skdif9ek34587fk30df6vm73==")).WillOnce(Invoke(
        [](const bzn::uuid_t& /*uuid*/, const std::string& /*key*/, const std::string& /*value*/)
        {
            return bzn::storage_base::result::ok;
        }));

    this->ch(request);
}


TEST_F(crud_test, test_that_a_leader_fails_to_create_an_existing_record)
{
    // record exists, don't bother with raft
    const std::string key{};
    const std::string value{"skdif9ek34587fk30df6vm73=="};
    auto request = generate_create_request(USER_UUID, key, value);

    // This node is in the leader state.
    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_storage, has(USER_UUID, key)).WillOnce(Return(true));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_RECORD_EXISTS);
            EXPECT_EQ(resp.header().transaction_id(), uint64_t(85746));
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_can_read_an_existing_record)
{
    // must attempt to read from storage and respond with value
    // if value does not exist in local storage, respond with error
    // and give leader uuid if it exists.

    auto request = generate_read_request(USER_UUID, "key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_storage, read(USER_UUID, "key0")).WillOnce(Invoke(
        [](const bzn::uuid_t& /*uuid*/, const std::string& /*key*/)
        {
            std::shared_ptr<bzn::storage_base::record> record = std::make_shared<bzn::storage_base::record>();
            record->value = "skdif9ek34587fk30df6vm73==";
            record->timestamp = std::chrono::seconds(0);
            record->transaction_id = TEST_NODE_UUID;
            return record;
        }));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().value(), "skdif9ek34587fk30df6vm73==");
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_apon_failing_to_read_suggests_leader)
{
    // same, but refer user to leader id.
    auto request = generate_read_request(USER_UUID, "key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillRepeatedly(Return(bzn::raft_state ::follower));

    EXPECT_CALL(*this->mock_storage, read(USER_UUID, "key0")).WillOnce(Return(nullptr));

    EXPECT_CALL(*this->mock_raft, get_leader()).WillRepeatedly(Return(bzn::peer_address_t("127.0.0.1",49152,8080,"ozzy",LEADER_UUID)));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kRedirect);
            EXPECT_EQ(resp.redirect().leader_id(), LEADER_UUID);
            EXPECT_EQ(resp.redirect().leader_host(), "127.0.0.1");
            EXPECT_EQ(resp.redirect().leader_port(), uint32_t(49152));
            EXPECT_EQ(resp.redirect().leader_name(), "ozzy");
            EXPECT_EQ(resp.redirect().leader_http_port(), uint32_t(8080));
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_candidate_read_fails)
{
    // there is an election respond with error
    auto msg = generate_read_request(USER_UUID, "key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::candidate));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kResp);
            EXPECT_EQ(resp.resp().error(), bzn::MSG_ELECTION_IN_PROGRESS);
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_read_existing_record)
{
    // must respond with data from storage, if it exists, if it does
    // not exist respond with data does not exist error.
    auto request = generate_read_request(USER_UUID,"key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_storage, read(USER_UUID, "key0")).WillOnce(Invoke(
        [](const bzn::uuid_t& /*uuid*/, const std::string& /*key*/)
        {
            auto record = std::make_shared<bzn::storage_base::record>();
            record->value = "skdif9ek34587fk30df6vm73==";
            record->timestamp = std::chrono::seconds(0);
            record->transaction_id = TEST_NODE_UUID;
            return record;
        }));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().value(), "skdif9ek34587fk30df6vm73==");
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_knowing_a_leader_attempting_update_fails)
{
    // must respond with error, and leader uuid if it
    auto request = generate_update_request(USER_UUID, "key0", "skdif9ek34587fk30df6vm73==");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_raft, get_leader()).WillOnce(Return(bzn::peer_address_t("127.0.0.1",49152,8080,"punkh",LEADER_UUID)));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kRedirect);
            EXPECT_EQ(resp.redirect().leader_id(), LEADER_UUID);
            EXPECT_EQ(resp.redirect().leader_host(), "127.0.0.1");
            EXPECT_EQ(resp.redirect().leader_port(), uint32_t(49152));
            EXPECT_EQ(resp.redirect().leader_name(), "punkh");
            EXPECT_EQ(resp.redirect().leader_http_port(), uint32_t(8080));
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_not_knowing_leader_update_fails)
{
    auto request = generate_update_request(USER_UUID, "key0", "skdif9ek34587fk30df6vm73==");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_raft, get_leader()).WillOnce(Return(bzn::peer_address_t("",0,0,"","")));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kRedirect);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_candidate_update_fails)
{
    auto msg = generate_update_request(USER_UUID, "key0", TEST_VALUE);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::candidate));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_ELECTION_IN_PROGRESS);
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_update)
{
    // if key-value pair exists, must respond with OK, and start the
    // update process via RAFT, otherwise error - key does not exist.
    const std::string key{"key0"};
    auto request = generate_update_request(USER_UUID, key, TEST_VALUE);

    // This node is in the leader state.
    EXPECT_CALL(*this->mock_raft, get_state()).WillRepeatedly(Return(bzn::raft_state::leader));

    EXPECT_CALL( *this->mock_storage, has(USER_UUID, key)).WillOnce(Return(true));

    EXPECT_CALL(*this->mock_raft, append_log(_)).WillOnce(Return(true));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.header().transaction_id(), uint64_t(85746));
        }));

    this->mh(request, this->mock_session);

    EXPECT_CALL(*this->mock_storage, update(USER_UUID, key, TEST_VALUE));

    this->ch(request);
}


TEST_F(crud_test, test_that_a_leader_cannot_update_a_record_that_does_not_exist)
{
    // if key-value pair exists, must respond with OK, and start the
    // update process via RAFT, otherwise error - key does not exist.
    const std::string key{"key0"};
    auto request = generate_update_request(USER_UUID, key, TEST_VALUE);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL( *this->mock_storage, has(USER_UUID, key)).WillOnce(Return(false));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_RECORD_NOT_FOUND);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_not_knowing_the_leader_delete_fails)
{
    auto request = generate_delete_request(USER_UUID, "key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_raft, get_leader()).WillOnce(Return(bzn::peer_address_t("",0,0,"","")));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kRedirect);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_knowing_the_leader_delete_fails)
{
    // must respond with error, and leader uuid
    auto request = generate_delete_request(USER_UUID, "key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    // We don't know the leader, maybe this node just started.
    EXPECT_CALL(*this->mock_raft, get_leader()).WillOnce(Return(bzn::peer_address_t("127.0.0.1", 49152,8080,"Pantera",LEADER_UUID)));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kRedirect);
            EXPECT_EQ(resp.redirect().leader_id(), LEADER_UUID);
            EXPECT_EQ(resp.redirect().leader_host(), "127.0.0.1");
            EXPECT_EQ(resp.redirect().leader_port(), uint32_t(49152));
            EXPECT_EQ(resp.redirect().leader_name(), "Pantera");
            EXPECT_EQ(resp.redirect().leader_http_port(), uint32_t(8080));
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_candidate_delete_fails)
{
    // must respond with error
    auto msg = generate_delete_request(USER_UUID, "key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::candidate));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_ELECTION_IN_PROGRESS);
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_delete_an_existing_record)
{
    // check for key, if the key does not exist send error message and bail
    // if the key does exist:
    //   respond with OK
    //   start the deletion process via RAFT
    //   append to log - do not commit
    //   do concensus via RAFT
    //   when RAFT calls commit handler perform delete

    const std::string key{"key0"};
    auto request = generate_delete_request(USER_UUID, key);

    // We tell CRUD that we are the leader
    EXPECT_CALL(*this->mock_raft, get_state()).WillRepeatedly(Return(bzn::raft_state::leader));

    // we fake a call to storage->has and tell CRUD that the record exists
    EXPECT_CALL(*this->mock_storage, has(USER_UUID, "key0")).WillOnce(Return(true));

    // since we do have a valid record to delete, we tell raft, raft will be cool with it...
    EXPECT_CALL(*this->mock_raft, append_log(_)).WillOnce(Return(true));

    // we respond to the user with OK.
    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), 0);
        }));

    // OK, run the message handler
    this->mh(request, mock_session);

    // at this point leader has accepted the request, parsed it, determined that
    // it's a delete command and sent the command to raft to send to the swarm,
    // apon reaching concensus RAFT will call the commit handler
    EXPECT_CALL(*this->mock_storage, remove(USER_UUID, key));

    this->ch(request);
}


TEST_F(crud_test, test_that_a_leader_fails_to_delete_an_nonexisting_record)
{
    const std::string key{"key0"};
    auto request = generate_delete_request(USER_UUID, key);

    // We tell CRUD that we are the leader
    EXPECT_CALL(*this->mock_raft, get_state()).WillRepeatedly(Return(bzn::raft_state::leader));

    // we fake a call to storage->has and tell CRUD that the record exists
    EXPECT_CALL(*this->mock_storage, has(USER_UUID, key)).WillOnce(Return(false));

    // we respond to the user with an error.
    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_RECORD_NOT_FOUND);
        }));

    // OK, run the message handler
    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_return_all_of_a_users_keys)
{
    // Ask local storage for all the keys for the user
    // package up the keys into a JSON array, send it back to the user
    auto request = generate_keys_request(USER_UUID);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_storage, get_keys(USER_UUID)).WillOnce(Invoke(
        [](const bzn::uuid_t& /*uuid*/)
        {
            return std::vector<std::string>{"key0", "key1", "key2"};
        }));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().keys().size(), int(3));
            EXPECT_EQ(resp.resp().keys(0), "key0");
            EXPECT_EQ(resp.resp().keys(1), "key1");
            EXPECT_EQ(resp.resp().keys(2), "key2");
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_follower_can_return_all_of_a_users_keys)
{
    auto request = generate_keys_request(USER_UUID);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_storage, get_keys(USER_UUID)).WillOnce(Invoke(
        [](const bzn::uuid_t& /*uuid*/)
        {
            return std::vector<std::string>{"key0", "key1", "key2"};
        }));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().keys().size(), int(3));
            EXPECT_EQ(resp.resp().keys(0), "key0");
            EXPECT_EQ(resp.resp().keys(1), "key1");
            EXPECT_EQ(resp.resp().keys(2), "key2");
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_follower_can_respond_to_has_command)
{
    // Ask local storage for all the keys for the user
    auto request = generate_has_request(USER_UUID, "key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_storage, has(USER_UUID, "key0")).WillOnce(Return(true));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kResp);
            ASSERT_TRUE(resp.resp().has());
        }));

    this->mh(request, mock_session);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_storage, has(USER_UUID, "key0")).WillOnce(Return(false));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kResp);
            ASSERT_FALSE(resp.resp().has());
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_respond_to_has_command)
{
    // Ask local storage for all the keys for the user
    auto request = generate_has_request(USER_UUID, "key0");

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_storage, has(USER_UUID, "key0")).WillOnce(Return(true));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kResp);
            ASSERT_TRUE(resp.resp().has());
        }));

    this->mh(request, mock_session);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_storage, has(USER_UUID, "key0")).WillOnce(Return(false));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), database_response::kResp);
            ASSERT_FALSE(resp.resp().has());
        }));


    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_leader_can_return_the_size_of_a_database)
{
    auto request = generate_size_request(USER_UUID);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::follower));

    EXPECT_CALL(*this->mock_storage, get_size(USER_UUID)).WillOnce(Return(123));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().size(), 123);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_follower_can_return_the_size_of_a_database)
{
    auto request = generate_size_request(USER_UUID);

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_storage, get_size(USER_UUID)).WillOnce(Return(123));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().size(), 123);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_CRUD_command_fails_when_not_given_bzn_api_or_cmd)
{
    auto request = generate_delete_request(USER_UUID, "key");

    request.removeMember("msg");

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_INVALID_CRUD_COMMAND);
        }));

    this->mh(request, this->mock_session);

    request["msg"] = "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ";

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_INVALID_CRUD_COMMAND);
        }));

    this->mh(request, this->mock_session);

    request["msg"] = ""; // message created is invalid

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_INVALID_ARGUMENTS);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_create_fails_if_the_value_size_exceeds_the_limit)
{
    bzn::message request = generate_create_request(USER_UUID,"large-value", std::string(bzn::MAX_VALUE_SIZE + 1, 'c'));

    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_VALUE_SIZE_TOO_LARGE);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_create_command_can_create_largest_value_record)
{
    bzn::message request = generate_create_request(USER_UUID, "key0", std::string(bzn::MAX_VALUE_SIZE, 'c'));

    EXPECT_CALL(*this->mock_raft, get_state()).WillRepeatedly(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_raft, append_log(_)).WillOnce(Return(true));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.success_case(), 0); // nothing set?
        }));

    EXPECT_CALL(*this->mock_storage, has(USER_UUID,"key0")).WillOnce(Return(false));

    this->mh(request, this->mock_session);

    EXPECT_CALL(*this->mock_storage, create(USER_UUID, "key0", std::string(bzn::MAX_VALUE_SIZE, 'c'))).WillOnce(Return(bzn::storage_base::result::ok));

    this->ch(request);
}


TEST_F(crud_test, test_that_a_update_command_fails_when_the_size_of_the_value_exceeds_the_value_size_limit)
{
    // if key-value pair exists, must respond with OK, and start the
    // update process via RAFT, otherwise error - key does not exist.
    bzn::message request = generate_update_request(USER_UUID, "key0", std::string(bzn::MAX_VALUE_SIZE + 1, 'c'));

    // This node is in the leader state.
    EXPECT_CALL(*this->mock_raft, get_state()).WillOnce(Return(bzn::raft_state::leader));

    EXPECT_CALL(*this->mock_session, send_message(An<std::shared_ptr<std::string>>(),_)).WillOnce(Invoke(
        [&](std::shared_ptr<std::string> msg, auto)
        {
            database_response resp;
            ASSERT_TRUE(resp.ParseFromString(*msg));
            EXPECT_EQ(resp.resp().error(), bzn::MSG_VALUE_SIZE_TOO_LARGE);
        }));

    this->mh(request, this->mock_session);
}
