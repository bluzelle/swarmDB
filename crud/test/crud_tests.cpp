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
    const bzn::uuid_t leader_uuid{"7110d050-42b3-11e8-842f-0ed5f89f718b"};
    const bzn::uuid_t user_uuid{"80174b53-2dda-49f1-9d6a-6a780d4cceca"};
    const std::string test_value = "I2luY2x1ZGUgPG1vY2tzL21vY2tfbm9kZV9iYXNlLmhwcD4NCiNpbmNsdWRlIDxtb2Nrcy9tb2NrX3Nlc3Npb25fYmFzZS5ocHA+DQojaW5jbHVkZSA8bW9ja3MvbW9ja19yYWZ0X2Jhc2UuaHBwPg0KI2luY2x1ZGUgPG1vY2tzL21vY2tfc3RvcmFnZV9iYXNlLmhwcD4NCg==";

    bzn::message generate_generic_message(const bzn::uuid_t& uid, const std::string& key, const std::string& cmd)
    {
        bzn::message msg;
        msg["cmd"] = cmd;
        msg["db-uuid"] = uid;
        msg["bzn-api"] = "crud";
        msg["request-id"] = 85746;
        if(key.size()>0)
        {
            msg["data"]["key"] = key;
        }
        return msg;
    }

    bzn::message generate_create_request(const bzn::uuid_t& uid, const std::string& key, const std::string& value)
    {
        bzn::message msg = generate_generic_message(uid, key, "create");
        msg["data"]["value"] = value;
        return msg;
    }

    bzn::message generate_read_request(const bzn::uuid_t& uid, const std::string& key)
    {
        return generate_generic_message(uid, key, "read");
    }

    bzn::message generate_update_request(const bzn::uuid_t& uid, const std::string& key, const std::string& value)
    {
        bzn::message msg = generate_generic_message(uid, key, "update");
        msg["data"]["value"] = value;
        return msg;
    }

    bzn::message generate_delete_request(const bzn::uuid_t& uid, const std::string& key)
    {
        return generate_generic_message(uid, key, "delete");
    }

    void set_leader_info(bzn::message& msg, const std::string& leader, const std::string& host, const uint16_t port, const std::string& name)
    {
        msg["data"]["leader-id"] = leader;
        msg["data"]["leader-host"] = host;
        msg["data"]["leader-port"] = port;
        msg["data"]["leader-name"] = name;
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

    void SetUp()
    {
        EXPECT_CALL(*this->mock_node, register_for_message("crud", _))
                .WillOnce(
                        Invoke([&](const std::string&, auto  mh){
                            this->mh = mh;
                            return true;
                        }));

        EXPECT_CALL(*mock_raft, register_commit_handler(_))
                .WillOnce(
                        Invoke([&](bzn::raft_base::commit_handler ch) { this->ch = ch; }));

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
    bzn::message request = generate_create_request(user_uuid, "key0", test_value);

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = 85746;
    (*expected_response)["error"] = bzn::MSG_NOT_THE_LEADER;

    set_leader_info(*expected_response, "", "", 0, "" );

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    // We don't know the leader, maybe this node just started.
    EXPECT_CALL(*this->mock_raft, get_leader())
            .WillOnce(Invoke([](){return bzn::peer_address_t("",0,"","");}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
//            LOG(debug) << "msg: " << (*msg).toStyledString();
//            LOG(debug) << "expected: " << (*expected_response).toStyledString();

            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_follower_knowing_leader_fails_to_create)
{
    // must respond with error, and respond with leader uuid
    bzn::message msg = generate_create_request(user_uuid, "key0", test_value);
    auto accepted_response = std::make_shared<bzn::message>();

    (*accepted_response)["request-id"] = msg["request-id"];
    (*accepted_response)["error"] = bzn::MSG_NOT_THE_LEADER;

    set_leader_info(*accepted_response, leader_uuid, "127.0.0.1", 49153, "iron maiden" );

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_raft, get_leader())
            .WillOnce(Invoke([&](){return bzn::peer_address_t("127.0.0.1",49153,"iron maiden",leader_uuid);}));


    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_candidate_fails_to_create)
{
    // must respond with error
    bzn::message msg = generate_create_request(user_uuid, "key0", test_value);

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = msg["request-id"];
    (*accepted_response)["error"] = bzn::MSG_ELECTION_IN_PROGRESS;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::candidate;}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_create_a_new_record)
{
    // must confirm with raft and create the record in storage
    // must respond with error
    bzn::message request = generate_create_request(user_uuid, "key0", "skdif9ek34587fk30df6vm73==");

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = request["request-id"];

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL(*this->mock_raft, append_log(_))
            .WillOnce(Return(true));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    EXPECT_CALL(*this->mock_storage, has(request["db-uuid"].asString(),request["data"]["key"].asString()))
            .WillOnce(Return(false));

    this->mh(request, this->mock_session);

    EXPECT_CALL(*this->mock_storage, create(request["db-uuid"].asString(), request["data"]["key"].asString(), request["data"]["value"].asString()))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/, const std::string& /*key*/, const std::string& /*value*/){return bzn::storage_base::result::ok;}
            ));

    this->ch(request);
}


TEST_F(crud_test, test_that_a_leader_fails_to_create_an_existing_record)
{
    // record exists, don't bother with raft
    const std::string key{"key0"};
    const std::string value{"skdif9ek34587fk30df6vm73=="};
    bzn::message request = generate_create_request(user_uuid, key, value);

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = request["request-id"];

    // This node is in the leader state.
    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    // has(const bzn::uuid_t& uuid, const  std::string& key)
    EXPECT_CALL(*this->mock_storage, has(user_uuid, key))
            .WillOnce(Return(false));

    EXPECT_CALL(*this->mock_raft, append_log(_));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);

    EXPECT_CALL(*this->mock_storage, create(request["db-uuid"].asString(), request["data"]["key"].asString(), request["data"]["value"].asString()))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/, const std::string& /*key*/, const std::string& /*value*/)
                    {return bzn::storage_base::result::exists;}));

    EXPECT_CALL(*this->mock_storage, error_msg(bzn::storage_base::result::exists));

    this->ch(request);

    // OK. We've already done a create, now redo the same create

    // This node is in the leader state.
    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    // has(const bzn::uuid_t& uuid, const  std::string& key)
    EXPECT_CALL(*this->mock_storage, has(user_uuid, key))
            .WillOnce(Return(true));

    expected_response->clear();
    (*expected_response)["error"] = bzn::MSG_RECORD_EXISTS;
    (*expected_response)["request-id"] = request["request-id"];

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_can_read_an_existing_record)
{
    // must attempt to read from storage and respond with value
    // if value does not exist in local storage, respond with error
    // and give leader uuid if it exists.

    bzn::message request = generate_read_request(user_uuid, "key0");

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = request["request-id"];
    (*expected_response)["data"]["value"] = "skdif9ek34587fk30df6vm73==";

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_storage,
                read(request["db-uuid"].asString(), request["data"]["key"].asString()))
            .WillOnce(Invoke([](const bzn::uuid_t& /*uuid*/, const std::string& /*key*/)
                    {
                        std::shared_ptr<bzn::storage_base::record> record = std::make_shared<bzn::storage_base::record>();
                        record->value = "skdif9ek34587fk30df6vm73==";
                        record->timestamp = std::chrono::seconds(0);
                        record->transaction_id = TEST_NODE_UUID;
                        return record;
                    }
            ));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_apon_failing_to_read_suggests_leader)
{
    // same, but refer user to leader id.
    bzn::message request = generate_read_request(user_uuid, "key0");

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = request["request-id"];
    (*expected_response)["error"] = bzn::MSG_VALUE_DOES_NOT_EXIST;

    set_leader_info(*expected_response, leader_uuid, "127.0.0.1", 49152, "ozzy" );

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillRepeatedly(Invoke([](){return bzn::raft_state ::follower;}));

    EXPECT_CALL(*this->mock_storage,
                read(request["db-uuid"].asString(), request["data"]["key"].asString()))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/, const std::string& /*key*/){return nullptr;}
                    ));

    EXPECT_CALL(*this->mock_raft, get_leader())
            .WillRepeatedly(Invoke([&](){return bzn::peer_address_t("127.0.0.1",49152,"ozzy",leader_uuid);}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_candidate_read_fails)
{
    // there is an election respond with error
    bzn::message msg = generate_read_request(user_uuid, "key0");
    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = msg["request-id"];
    (*accepted_response)["error"] = bzn::MSG_ELECTION_IN_PROGRESS;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(
                    Invoke([](){return bzn::raft_state::candidate;}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_read_existing_record)
{
    // must respond with data from storage, if it exists, if it does
    // not exist respond with data does not exist error.
    bzn::message request = generate_read_request(user_uuid,"key0");

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = request["request-id"];
    (*expected_response)["data"]["value"] = "skdif9ek34587fk30df6vm73==";

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL(*this->mock_storage,
                read(request["db-uuid"].asString(), request["data"]["key"].asString()))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/, const std::string& /*key*/)
                    {
                        std::shared_ptr<bzn::storage_base::record> record = std::make_shared<bzn::storage_base::record>();
                        record->value = "skdif9ek34587fk30df6vm73==";
                        record->timestamp = std::chrono::seconds(0);
                        record->transaction_id = TEST_NODE_UUID;
                        return record;
                    }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_knowing_a_leader_attempting_update_fails)
{
    // must respond with error, and leader uuid if it
    bzn::message request = generate_update_request(user_uuid, "key0", "skdif9ek34587fk30df6vm73==");
    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["error"] = bzn::MSG_NOT_THE_LEADER;

    set_leader_info(*accepted_response, leader_uuid, "127.0.0.1", 49152, "punkh" );

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_raft, get_leader())
            .WillOnce(Invoke([&](){return bzn::peer_address_t("127.0.0.1",49152,"punkh",leader_uuid);}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_not_knowing_leader_update_fails)
{
    bzn::message request = generate_update_request(user_uuid, "key0", "skdif9ek34587fk30df6vm73==");

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["error"] = bzn::MSG_NOT_THE_LEADER;

    set_leader_info(*accepted_response, "", "", 0, "" );

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_raft, get_leader())
            .WillOnce(Invoke([&](){return bzn::peer_address_t("",0,"","");}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_candidate_update_fails)
{
    bzn::message msg = generate_update_request(user_uuid, "key0", test_value);

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = msg["request-id"];
    (*accepted_response)["error"] = bzn::MSG_ELECTION_IN_PROGRESS;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::candidate;}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(msg, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_update)
{
    // if key-value pair exists, must respond with OK, and start the
    // update process via RAFT, otherwise error - key does not exist.
    const std::string key{"key0"};
    bzn::message request = generate_update_request(user_uuid, key, test_value);

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = request["request-id"];

    // This node is in the leader state.
    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL( *this->mock_storage, has(user_uuid, key))
            .WillOnce(Return(true));

    EXPECT_CALL(*this->mock_raft, append_log(_))
            .WillOnce(Return(true));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));


    this->mh(request, this->mock_session);

    EXPECT_CALL(*this->mock_storage, update(user_uuid, key, test_value));

    this->ch(request);
}


TEST_F(crud_test, test_that_a_leader_cannot_update_a_record_that_does_not_exist)
{
    // if key-value pair exists, must respond with OK, and start the
    // update process via RAFT, otherwise error - key does not exist.
    const std::string key{"key0"};
    bzn::message request = generate_update_request(user_uuid, key, test_value);

    auto expected_response = std::make_shared<bzn::message>();

    // "{\n\t\"error\" : \"RECORD_NOT_FOUND\",\n\t\"request-id\" : 85746\n}\n"
    (*expected_response)["request-id"] = request["request-id"];
    (*expected_response)["error"] = bzn::MSG_RECORD_NOT_FOUND;

    // This node is in the leader state.
    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL( *this->mock_storage, has(user_uuid, key))
            .WillOnce(Return(false));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_not_knowing_the_leader_delete_fails)
{
    // must respond with error
    bzn::message request = generate_delete_request(user_uuid, "key0");

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = 85746;
    (*expected_response)["error"] = bzn::MSG_NOT_THE_LEADER;

    set_leader_info(*expected_response, "", "", 0, "" );

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    // We don't know the leader, maybe this node just started.
    EXPECT_CALL(*this->mock_raft, get_leader())
            .WillOnce(Invoke([](){return bzn::peer_address_t("",0,"","");}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_follower_knowing_the_leader_delete_fails)
{
    // must respond with error, and leader uuid
    bzn::message request = generate_delete_request(user_uuid, "key0");

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = 85746;
    (*expected_response)["error"] = bzn::MSG_NOT_THE_LEADER;

    set_leader_info(*expected_response, leader_uuid, "127.0.0.1", 49152, "Pantera" );

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    // We don't know the leader, maybe this node just started.
    EXPECT_CALL(*this->mock_raft, get_leader())
            .WillOnce(Invoke([](){return bzn::peer_address_t("127.0.0.1", 49152,"Pantera",leader_uuid);}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*expected_response, *msg);
        }));

    this->mh(request, this->mock_session);
}


TEST_F(crud_test, test_that_a_candidate_delete_fails)
{
    // must respond with error
    bzn::message msg = generate_delete_request(user_uuid, "key0");

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = msg["request-id"];
    (*accepted_response)["error"] = bzn::MSG_ELECTION_IN_PROGRESS;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(
                    Invoke([](){return bzn::raft_state::candidate;}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
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
    bzn::message request = generate_delete_request(user_uuid, key);

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];

    // We tell CRUD that we are the leader
    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    // we fake a call to storage->has and tell CRUD that the record exists
    EXPECT_CALL(*this->mock_storage, has(request["db-uuid"].asString(),request["data"]["key"].asString()))
            .WillOnce(Return(true));

    // since we do have a valid record to delete, we tell raft, raft will be cool with it...
    EXPECT_CALL(*this->mock_raft, append_log(_))
            .WillOnce(Return(true));

    // we respond to the user with OK.
    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    // OK, run the message handler
    this->mh(request, mock_session);

    // at this point leader has accepted the request, parsed it, determined that
    // it's a delete command and sent the command to raft to send to the swarm,
    // apon reaching concensus RAFT will call the commit handler
    EXPECT_CALL(*this->mock_storage, remove(user_uuid, key));
    this->ch(request);
}


TEST_F(crud_test, test_that_a_leader_fails_to_delete_an_nonexisting_record)
{
    const std::string key{"key0"};
    bzn::message request = generate_delete_request(user_uuid, key);

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["error"] = bzn::MSG_RECORD_NOT_FOUND;

    // We tell CRUD that we are the leader
    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    // we fake a call to storage->has and tell CRUD that the record exists
    EXPECT_CALL(*this->mock_storage, has(request["db-uuid"].asString(),request["data"]["key"].asString()))
            .WillOnce(Return(false));

    // we respond to the user with an error.
    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    // OK, run the message handler
    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_return_all_of_a_users_keys)
{
    // Ask local storage for all the keys for the user
    // package up the keys into a JSON array, send it back to the user

    const std::string key{"key0"};
    bzn::message request = generate_generic_message(user_uuid, "", "keys");

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    Json::Value key_array;
    key_array.append("key0");
    key_array.append("key1");
    key_array.append("key2");

    (*accepted_response)["data"]["keys"] = key_array;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL(*this->mock_storage, get_keys(user_uuid))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/)
                    {
                        return std::vector<std::string>{"key0", "key1", "key2"};
                    }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_follower_can_return_all_of_a_users_keys)
{
    // Ask local storage for all the keys for the user
    // package up the keys into a JSON array, send it back to the user
    const std::string key{"key0"};
    bzn::message request = generate_generic_message(user_uuid, "", "keys");

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    Json::Value key_array;
    key_array.append("key0");
    key_array.append("key1");
    key_array.append("key2");

    (*accepted_response)["data"]["keys"] = key_array;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_storage, get_keys(user_uuid))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/)
                    {
                        return std::vector<std::string>{"key0", "key1", "key2"};
                    }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_follower_can_respond_to_has_command)
{
    // Ask local storage for all the keys for the user
    // package up the keys into a JSON array, send it back to the user
    bzn::message request = generate_generic_message(user_uuid, "", "has");
    request["data"]["key"] = "key0";

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["data"]["key-exists"] = true;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_storage, has(user_uuid, "key0"))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/, std::string)
                    {
                        return true;
                    }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);

    (*accepted_response)["data"]["key-exists"] = false;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_storage, has(user_uuid, "key0"))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/, std::string)
                    {
                        return false;
                    }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_follower_can_respond_to_has_command_with_bad_args)
{
    // Ask local storage for all the keys for the user
    // package up the keys into a JSON array, send it back to the user
    bzn::message request = generate_generic_message(user_uuid, "", "has");

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["error"] = bzn::MSG_INVALID_ARGUMENTS;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_respond_to_has_command)
{
    // Ask local storage for all the keys for the user
    // package up the keys into a JSON array, send it back to the user
    bzn::message request = generate_generic_message(user_uuid, "", "has");
    request["data"]["key"] = "key0";

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["data"]["key-exists"] = true;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL(*this->mock_storage, has(user_uuid, "key0"))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/, std::string)
                    {
                        return true;
                    }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);

    (*accepted_response)["data"]["key-exists"] = false;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL(*this->mock_storage, has(user_uuid, "key0"))
            .WillOnce(Invoke(
                    [](const bzn::uuid_t& /*uuid*/, std::string)
                    {
                        return false;
                    }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));


    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_leader_can_respond_to_has_command_with_bad_args)
{
    // Ask local storage for all the keys for the user
    // package up the keys into a JSON array, send it back to the user
    bzn::message request = generate_generic_message(user_uuid, "", "has");

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["error"] = bzn::MSG_INVALID_ARGUMENTS;

    EXPECT_CALL(*this->mock_raft, get_state())
            .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_leader_can_return_the_size_of_a_database)
{
    const std::string key{"key0"};
    bzn::message request = generate_generic_message(user_uuid, "", "size");

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["data"]["size"] = Json::UInt64(123);

    EXPECT_CALL(*this->mock_raft, get_state())
        .WillOnce(Invoke([](){return bzn::raft_state::follower;}));

    EXPECT_CALL(*this->mock_storage, get_size(user_uuid))
        .WillOnce(Invoke(
            [](const bzn::uuid_t& /*uuid*/)
            {
                return 123;
            }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_follower_can_return_the_size_of_a_database)
{
    const std::string key{"key0"};
    bzn::message request = generate_generic_message(user_uuid, "", "size");

    auto accepted_response = std::make_shared<bzn::message>();
    (*accepted_response)["request-id"] = request["request-id"];
    (*accepted_response)["data"]["size"] = Json::UInt64(123);

    EXPECT_CALL(*this->mock_raft, get_state())
        .WillOnce(Invoke([](){return bzn::raft_state::leader;}));

    EXPECT_CALL(*this->mock_storage, get_size(user_uuid))
        .WillOnce(Invoke(
            [](const bzn::uuid_t& /*uuid*/)
            {
                return 123;
            }));

    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
        [&](auto& msg, auto)
        {
            EXPECT_EQ(*accepted_response, *msg);
        }));

    this->mh(request, mock_session);
}


TEST_F(crud_test, test_that_a_create_fails_when_not_given_required_parameters)
{
    bzn::message request = generate_create_request(user_uuid, "key", "skdif9ek34587fk30df6vm73==");

    auto expected_response = std::make_shared<bzn::message>();
    (*expected_response)["request-id"] = request["request-id"];
    (*expected_response)["error"] = bzn::MSG_INVALID_CRUD_COMMAND;
    bzn::raft_state raft_state = bzn::raft_state::leader;


    request.removeMember("bzn-api");
    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
            [&](auto& msg, auto)
            {
                EXPECT_EQ(*expected_response, *msg);
            }));

    this->mh(request, this->mock_session);
    request["bzn-api"] = "crud";

    request.removeMember("cmd");
    EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
            [&](auto& msg, auto)
            {
                EXPECT_EQ(*expected_response, *msg);
            }));
    this->mh(request, this->mock_session);
    request["cmd"] = "create";

    (*expected_response)["error"] = bzn::MSG_INVALID_ARGUMENTS;

    auto perform_test = [&](){
        EXPECT_CALL(*this->mock_raft, get_state())
                .WillOnce(Invoke([&](){return raft_state;}));

        EXPECT_CALL(*this->mock_session, send_message(_,_)).WillOnce(Invoke(
                [&](auto& msg, auto)
                {
                    EXPECT_EQ(*expected_response, *msg);
                }));

        this->mh(request, this->mock_session);
    };

    request["data"].removeMember("key");
    perform_test();
    request["data"]["key"] = "key";

    request["data"].removeMember("value");
    perform_test();
    request["data"]["value"] = "datavalue";

    request.removeMember("db-uuid");
    perform_test();
    request["data"]["value"] = user_uuid;

    request.removeMember("data");
    perform_test();
}
