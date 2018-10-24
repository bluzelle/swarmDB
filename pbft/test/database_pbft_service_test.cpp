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

#include <mocks/mock_boost_asio_beast.hpp>
#include <mocks/mock_storage_base.hpp>
#include <mocks/mock_session_base.hpp>
#include <pbft/database_pbft_service.hpp>
#include <storage/mem_storage.hpp>

using namespace ::testing;

namespace
{
    const std::string TEST_UUID{"uuid"};
    const std::string DEFAULT_NEXT_REQUEST_SEQUENCE{"1"};
}


TEST(database_pbft_service, test_that_on_construction_if_next_request_sequence_doesnt_exist_its_created)
{
    auto mock_storage = std::make_shared<bzn::Mockstorage_base>();

    EXPECT_CALL(*mock_storage, read(_, _)).WillOnce(Return(std::optional<bzn::value_t>()));
    EXPECT_CALL(*mock_storage, create(_, _, DEFAULT_NEXT_REQUEST_SEQUENCE)).WillOnce(Return(bzn::storage_base::result::ok));
    EXPECT_CALL(*mock_storage, update(_, _, DEFAULT_NEXT_REQUEST_SEQUENCE)).WillOnce(Return(bzn::storage_base::result::ok));

    bzn::database_pbft_service dps(std::make_shared<bzn::asio::Mockio_context_base>(), mock_storage, TEST_UUID);
}


TEST(database_pbft_service, test_that_on_construction_if_next_request_sequence_exists_its_loaded)
{
    auto mock_storage = std::make_shared<bzn::Mockstorage_base>();

    EXPECT_CALL(*mock_storage, read(_, _)).WillOnce(Return(std::optional<bzn::value_t>("123")));
    EXPECT_CALL(*mock_storage, update(_, _, "123")).WillOnce(Return(bzn::storage_base::result::ok));

    bzn::database_pbft_service dps(std::make_shared<bzn::asio::Mockio_context_base>(), mock_storage, TEST_UUID);
}


TEST(database_pbft_service, test_that_on_construction_if_next_request_sequence_doesnt_exist_it_throws_if_error_occurs)
{
    auto mock_storage = std::make_shared<bzn::Mockstorage_base>();

    EXPECT_CALL(*mock_storage, read(_, _)).WillOnce(Return(std::optional<bzn::value_t>()));
    EXPECT_CALL(*mock_storage, create(_, _, DEFAULT_NEXT_REQUEST_SEQUENCE)).WillOnce(Return(bzn::storage_base::result::value_too_large));

    EXPECT_THROW(bzn::database_pbft_service dps(std::make_shared<bzn::asio::Mockio_context_base>(), mock_storage, TEST_UUID), std::runtime_error);
}


TEST(database_pbft_service, test_that_failed_storing_of_operation_throws)
{
    auto mock_storage = std::make_shared<bzn::Mockstorage_base>();

    EXPECT_CALL(*mock_storage, read(_, _)).WillOnce(Return(std::optional<bzn::value_t>()));
    EXPECT_CALL(*mock_storage, create(_, _, DEFAULT_NEXT_REQUEST_SEQUENCE)).WillOnce(Return(bzn::storage_base::result::ok));

    bzn::database_pbft_service dps(std::make_shared<bzn::asio::Mockio_context_base>(), mock_storage, TEST_UUID);

    EXPECT_CALL(*mock_storage, create(_, _, _)).WillOnce(Return(bzn::storage_base::result::exists));
    EXPECT_CALL(*mock_storage, update(_, _, _)).WillOnce(Return(bzn::storage_base::result::ok));

    pbft_request msg;
    auto operation = std::make_shared<bzn::pbft_operation>(0, 1, msg, nullptr);

    EXPECT_THROW(dps.apply_operation(operation), std::runtime_error);
}


TEST(database_pbft_service, test_that_stored_operation_is_executed_in_order_and_registered_handler_is_scheduled)
{
    auto mem_storage = std::make_shared<bzn::mem_storage>();
    auto mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();

    bzn::database_pbft_service dps(mock_io_context, mem_storage, TEST_UUID);

    pbft_request msg;

    msg.mutable_operation()->mutable_header()->set_db_uuid(TEST_UUID);
    msg.mutable_operation()->mutable_header()->set_transaction_id(uint64_t(123));
    msg.mutable_operation()->mutable_create()->set_key("key2");
    msg.mutable_operation()->mutable_create()->set_value("value2");

    auto operation2 = std::make_shared<bzn::pbft_operation>(0, 2, msg, nullptr);

    dps.apply_operation(operation2);

    ASSERT_EQ(uint64_t(0), dps.applied_requests_count());

    msg.mutable_operation()->mutable_header()->set_transaction_id(uint64_t(321));
    msg.mutable_operation()->mutable_create()->set_key("key1");
    msg.mutable_operation()->mutable_create()->set_value("value1");

    auto operation1 = std::make_shared<bzn::pbft_operation>(0, 1, msg, nullptr);

    EXPECT_CALL(*mock_io_context, post(_)).Times(2);

    // todo: once crud is in we can test for proper ordering of db calls...

    dps.apply_operation(operation1);

    ASSERT_EQ(uint64_t(2), dps.applied_requests_count());
}
