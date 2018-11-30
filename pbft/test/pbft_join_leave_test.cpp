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

#include <pbft/test/pbft_test_common.hpp>
#include <utils/make_endpoint.hpp>
#include <pbft/test/pbft_proto_test.hpp>
#include <utils/bytes_to_debug_string.hpp>

using namespace ::testing;

namespace bzn
{
    using namespace bzn::test;
    const bzn::peer_address_t new_peer{"127.0.0.1", 8090, 83, "name_new", "uuid_new"};
    const bzn::peer_address_t new_peer2{"127.0.0.1", 8091, 84, "name_new2", "uuid_new2"};

    std::optional<pbft_msg> extract_pbft_msg_option(bzn_envelope message)
    {
        if (message.payload_case() == bzn_envelope::kPbft)
        {
            pbft_msg pmsg;
            if (pmsg.ParseFromString(message.pbft()))
            {
                return pmsg;
            }
        }
        return {};
    }

    MATCHER_P(message_has_correct_req_hash, req_hash, "")
    {
        auto pmsg = extract_pbft_msg_option(*arg);
        if (pmsg)
        {
            return pmsg->request_hash() == req_hash;
        }
        return false;
    }

    MATCHER_P(message_has_correct_pbft_type, pbft_type, "")
    {
        auto pmsg = extract_pbft_msg_option(*arg);
        if (pmsg)
        {
            return pmsg->type() == pbft_type;
        }
        return false;
    }

    MATCHER_P(message_has_req_with_correct_type, req_type, "")
    {
        auto pmsg = extract_pbft_msg_option(*arg);
        if (pmsg)
        {
            if (pmsg->has_request())
            {
                return pmsg->request().payload_case() == req_type;
            }
        }
        return false;
    }

    namespace test
    {
        pbft_msg
        send_new_config_preprepare(std::shared_ptr<bzn::pbft> pbft, std::shared_ptr<bzn::Mocknode_base> mock_node
            , const bzn::pbft_configuration &config)
        {
            // make and "send" a pre-prepare message for a new_config
            auto req = new bzn_envelope;
            auto cfg_msg = new pbft_config_msg;
            cfg_msg->set_configuration(config.to_string());
            req->set_pbft_internal_request(cfg_msg->SerializeAsString());

            pbft_msg preprepare;
            preprepare.set_view(1);
            preprepare.set_sequence(100);
            preprepare.set_type(PBFT_MSG_PREPREPARE);
            preprepare.set_allocated_request(new bzn_envelope(*req));
            auto crypto = std::make_shared<bzn::crypto>(std::make_shared<bzn::options>());
            auto expect_hash = crypto->hash(preprepare.request());
            preprepare.set_request_hash(expect_hash);

            // receiving node should send out prepare messsage to everyone
            for (auto const &p : TEST_PEER_LIST)
            {
                EXPECT_CALL(*(mock_node),
                    send_message(bzn::make_endpoint(p),
                        AllOf(message_has_correct_req_hash(expect_hash), message_has_correct_pbft_type(PBFT_MSG_PREPARE))))
                    .Times(Exactly(1));
            }

            auto wmsg = wrap_pbft_msg(preprepare);
            wmsg.set_sender("bob");
            pbft->handle_message(preprepare, wmsg);
            return preprepare;
        }

        void
        send_new_config_prepare(std::shared_ptr<bzn::pbft> pbft, bzn::peer_address_t node, std::shared_ptr<pbft_operation> op)
        {
            pbft_msg prepare;
            prepare.set_view(op->get_view());
            prepare.set_sequence(op->get_sequence());
            prepare.set_type(PBFT_MSG_PREPARE);
            prepare.set_request_hash(op->get_request_hash());

            auto wmsg = wrap_pbft_msg(prepare);
            wmsg.set_sender(node.uuid);
            pbft->handle_message(prepare, wmsg);
        }

        void
        send_new_config_commit(std::shared_ptr<bzn::pbft> pbft, bzn::peer_address_t node, std::shared_ptr<pbft_operation> op)
        {
            pbft_msg commit;
            commit.set_view(op->get_view());
            commit.set_sequence(op->get_sequence());
            commit.set_type(PBFT_MSG_COMMIT);
            commit.set_request_hash(op->get_request_hash());

            auto wmsg = wrap_pbft_msg(commit);
            wmsg.set_sender(node.uuid);
            pbft->handle_message(commit, wmsg);
        }
    }

    TEST_F(pbft_test, join_request_generates_new_config_preprepare)
    {
        this->build_pbft();

        auto info = new pbft_peer_info;
        info->set_host(new_peer.host);
        info->set_name(new_peer.name);
        info->set_port(new_peer.port);
        info->set_http_port(new_peer.http_port);
        info->set_uuid(new_peer.uuid);

        pbft_membership_msg join_msg;
        join_msg.set_type(PBFT_MMSG_JOIN);
        join_msg.set_allocated_peer_info(info);

        // each peer should be sent a pre-prepare for new_config when the join is received
        for (auto const &p : TEST_PEER_LIST)
        {
            EXPECT_CALL(*(this->mock_node),
                send_message(bzn::make_endpoint(p),
                    AllOf(message_has_req_with_correct_type(bzn_envelope::kPbftInternalRequest), message_has_correct_pbft_type(PBFT_MSG_PREPREPARE))))
                .Times(Exactly(1));
        }

        auto wmsg = wrap_pbft_membership_msg(join_msg);
        wmsg.set_sender(new_peer.uuid);
        this->pbft->handle_membership_message(wmsg);
    }

    TEST_F(pbft_test, valid_leave_request_test)
    {
        this->build_pbft();
        auto const& peer = *(TEST_PEER_LIST.begin());
        auto info = new pbft_peer_info;
        info->set_host(peer.host);
        info->set_name(peer.name);
        info->set_port(peer.port);
        info->set_http_port(peer.http_port);
        info->set_uuid(peer.uuid);

        pbft_membership_msg leave_msg;
        leave_msg.set_type(PBFT_MMSG_LEAVE);
        leave_msg.set_allocated_peer_info(info);

        // each peer should be sent a pre-prepare for new_config when the leave is received
        for (auto const &p : TEST_PEER_LIST)
        {
            EXPECT_CALL(*(this->mock_node),
                    send_message(bzn::make_endpoint(p),
                            AllOf(message_has_req_with_correct_type(bzn_envelope::kPbftInternalRequest), message_has_correct_pbft_type(PBFT_MSG_PREPREPARE))))
                    .Times(Exactly(1));
        }

        auto wmsg = wrap_pbft_membership_msg(leave_msg);
        wmsg.set_sender(peer.uuid);
        this->pbft->handle_membership_message(wmsg);
    }

    TEST_F(pbft_test, invalid_leave_request_test)
    {
        this->build_pbft();

        auto info = new pbft_peer_info;
        info->set_host(new_peer.host);
        info->set_name(new_peer.name);
        info->set_port(new_peer.port);
        info->set_http_port(new_peer.http_port);
        info->set_uuid(new_peer.uuid);

        pbft_membership_msg leave_msg;
        leave_msg.set_type(PBFT_MMSG_LEAVE);
        leave_msg.set_allocated_peer_info(info);

        // leave message should be ignored

        auto wmsg = wrap_pbft_membership_msg(leave_msg);
        wmsg.set_sender(new_peer.uuid);
        this->pbft->handle_membership_message(wmsg);
    }

    TEST_F(pbft_test, test_new_config_preprepare_handling)
    {
        this->build_pbft();

        pbft_configuration config;
        config.add_peer(new_peer);

        send_new_config_preprepare(pbft, this->mock_node, config);

        // the config should now be stored by this node, but not marked enabled/current
        ASSERT_NE(this->pbft->configurations.get(config.get_hash()), nullptr);
        EXPECT_FALSE(this->pbft->configurations.is_enabled(config.get_hash()));
        EXPECT_NE(*(this->pbft->configurations.current()), config);
        EXPECT_FALSE(this->pbft->is_configuration_acceptable_in_new_view(config.get_hash()));
    }

    TEST_F(pbft_test, test_new_config_prepare_handling)
    {
        this->build_pbft();

        // PREPREPARE step
        pbft_configuration config;
        config.add_peer(new_peer);
        auto msg = send_new_config_preprepare(pbft, this->mock_node, config);

        LOG(info) << bytes_to_debug_string(msg.request_hash());

        auto op = this->pbft->find_operation(msg);
        ASSERT_NE(op, nullptr);

        // PREPARE step
        auto nodes = TEST_PEER_LIST.begin();
        size_t req_nodes = pbft::honest_majority_size(TEST_PEER_LIST.size());
        for (size_t i = 0; i < req_nodes - 1; i++)
        {
            bzn::peer_address_t node(*nodes++);
            send_new_config_prepare(pbft, node, op);
        }

        // the next prepare should trigger a commit message
        for (auto const &p : TEST_PEER_LIST)
        {
            EXPECT_CALL(*(mock_node),
                send_message(bzn::make_endpoint(p),
                    AllOf(message_has_correct_req_hash(msg.request_hash()), message_has_correct_pbft_type(PBFT_MSG_COMMIT))))
                .Times(Exactly(1));
        }

        bzn::peer_address_t node(*nodes++);
        send_new_config_prepare(pbft, node, op);

        // and now the config should be enabled and acceptable, but not current
        ASSERT_NE(this->pbft->configurations.get(config.get_hash()), nullptr);
        EXPECT_TRUE(this->pbft->configurations.is_enabled(config.get_hash()));
        EXPECT_NE(*(this->pbft->configurations.current()), config);
        EXPECT_TRUE(this->pbft->is_configuration_acceptable_in_new_view(config.get_hash()));
    }

    TEST_F(pbft_test, test_new_config_commit_handling)
    {
        this->build_pbft();

        // insert and enable a dummy configuration prior to the new one to be proposed
        auto dummy_config = std::make_shared<pbft_configuration>();
        dummy_config->add_peer(new_peer2);
        EXPECT_TRUE(this->pbft->configurations.add(dummy_config));
        this->pbft->configurations.enable(dummy_config->get_hash());
        EXPECT_TRUE(this->pbft->is_configuration_acceptable_in_new_view(dummy_config->get_hash()));

        // PREPREPARE step
        pbft_configuration config;
        config.add_peer(new_peer);
        auto msg = send_new_config_preprepare(pbft, this->mock_node, config);

        auto op = this->pbft->find_operation(msg);
        ASSERT_NE(op, nullptr);

        // PREPARE step
        for (auto const &p : TEST_PEER_LIST)
        {
            EXPECT_CALL(*(mock_node),
                send_message(bzn::make_endpoint(p),
                    AllOf(message_has_correct_req_hash(msg.request_hash()), message_has_correct_pbft_type(PBFT_MSG_COMMIT))))
                .Times(Exactly(1));
        }

        for (auto const &p : TEST_PEER_LIST)
        {
            send_new_config_prepare(pbft, p, op);
        }

        // COMMIT step
        auto nodes = TEST_PEER_LIST.begin();
        size_t req_nodes = pbft::honest_majority_size(TEST_PEER_LIST.size());
        for (size_t i = 0; i < req_nodes - 1; i++)
        {
            bzn::peer_address_t node(*nodes++);
            send_new_config_commit(pbft, node, op);
        }
        EXPECT_TRUE(this->pbft->is_configuration_acceptable_in_new_view(dummy_config->get_hash()));

        // one more commit should do it...
        bzn::peer_address_t node(*nodes++);
        send_new_config_commit(pbft, node, op);

        // and now the config should be enabled and acceptable
        ASSERT_NE(this->pbft->configurations.get(config.get_hash()), nullptr);
        EXPECT_TRUE(this->pbft->configurations.is_enabled(config.get_hash()));
        EXPECT_NE(*(this->pbft->configurations.current()), config);
        EXPECT_TRUE(this->pbft->is_configuration_acceptable_in_new_view(config.get_hash()));

        // and no earlier one should be present (apart from the current configuration)
        EXPECT_EQ(this->pbft->configurations.get(dummy_config->get_hash()), nullptr);
        EXPECT_FALSE(this->pbft->is_configuration_acceptable_in_new_view(dummy_config->get_hash()));
    }

    TEST_F(pbft_test, test_move_to_new_config)
    {
        this->build_pbft();

        auto current_config = this->pbft->configurations.current();
        auto config = std::make_shared<pbft_configuration>();
        config->add_peer(new_peer);
        this->pbft->configurations.add(config);
        EXPECT_FALSE(this->pbft->move_to_new_configuration(config->get_hash()));

        this->pbft->configurations.enable(config->get_hash());
        EXPECT_TRUE(this->pbft->move_to_new_configuration(config->get_hash()));

        // previous configuration should have been removed
        EXPECT_EQ(this->pbft->configurations.get(current_config->get_hash()), nullptr);
    }
}