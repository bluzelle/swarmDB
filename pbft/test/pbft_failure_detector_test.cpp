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

#include <gtest/gtest.h>
#include <include/bluzelle.hpp>
#include <pbft/pbft_failure_detector.hpp>
#include <proto/bluzelle.pb.h>
#include <mocks/mock_boost_asio_beast.hpp>

using namespace ::testing;

namespace bzn
{

    class pbft_failure_detector_test : public Test
    {
    public:
        std::shared_ptr<bzn::pbft_failure_detector> failure_detector;

        std::shared_ptr<bzn::asio::mock_io_context_base> mock_io_context =
                std::make_shared<NiceMock<bzn::asio::mock_io_context_base>>();
        std::unique_ptr<bzn::asio::mock_steady_timer_base> request_timer =
                std::make_unique<NiceMock<bzn::asio::mock_steady_timer_base>>();

        bzn::asio::wait_handler request_timer_callback;

        bool failure_detected = false;
        void failure_detect_handler()
        {
            this->failure_detected = true;
        }

        bzn::hash_t req_a = "a";
        bzn::hash_t req_b = "b";

        void build_failure_detector()
        {
            this->failure_detector = std::make_shared<bzn::pbft_failure_detector>(this->mock_io_context);
            this->failure_detector
                ->register_failure_handler(std::bind(&pbft_failure_detector_test::failure_detect_handler, this));
        }

        pbft_failure_detector_test()
        {
            EXPECT_CALL(*(this->mock_io_context), make_unique_steady_timer())
                    .WillOnce(Invoke(
                            [&](){return std::move(this->request_timer);}
                    ));

            EXPECT_CALL(*(this->request_timer), async_wait(_))
                    .Times(AnyNumber())
                    .WillRepeatedly(Invoke(
                            [&](auto handler){this->request_timer_callback = handler;}
                    ));
        }
    };

    TEST_F(pbft_failure_detector_test, first_request_starts_timer)
    {
        EXPECT_CALL(*(this->request_timer), expires_from_now(_)).Times(Exactly(1));
        this->build_failure_detector();

        this->failure_detector->request_seen(req_a);
    }

    TEST_F(pbft_failure_detector_test, second_request_doesnt_start_timer)
    {
        EXPECT_CALL(*(this->request_timer), expires_from_now(_)).Times(Exactly(1));
        this->build_failure_detector();

        this->failure_detector->request_seen(req_a);
        this->failure_detector->request_seen(req_b);
    }

    TEST_F(pbft_failure_detector_test, no_timer_while_queue_empty)
    {
        EXPECT_CALL(*(this->request_timer), expires_from_now(_)).Times(Exactly(1));
        this->build_failure_detector();

        this->failure_detector->request_seen(req_a);
        this->failure_detector->request_executed(req_a);
        this->request_timer_callback(boost::system::error_code());
    }

    TEST_F(pbft_failure_detector_test, timer_restarted_after_cleared)
    {
        EXPECT_CALL(*(this->request_timer), expires_from_now(_)).Times(Exactly(2));
        this->build_failure_detector();

        this->failure_detector->request_seen(req_a);
        this->failure_detector->request_executed(req_a);
        this->request_timer_callback(boost::system::error_code());
        this->failure_detector->request_seen(req_b);
    }

    TEST_F(pbft_failure_detector_test, duplicate_requests_ignored)
    {
        EXPECT_CALL(*(this->request_timer), expires_from_now(_)).Times(Exactly(1));
        this->build_failure_detector();

        this->failure_detector->request_seen(req_a);
        this->failure_detector->request_executed(req_a);
        this->failure_detector->request_seen(req_a);
    }

    TEST_F(pbft_failure_detector_test, timeout_triggers_callback)
    {
        EXPECT_CALL(*(this->mock_io_context), post(_)).WillOnce(Invoke(std::bind(&pbft_failure_detector_test::failure_detect_handler, this)));
        this->build_failure_detector();

        this->failure_detector->request_seen(req_a);

        ASSERT_FALSE(this->failure_detected);
        this->request_timer_callback(boost::system::error_code());
        ASSERT_TRUE(this->failure_detected);
    }

    TEST_F(pbft_failure_detector_test, timeout_doesnt_restart_timer)
    {
        EXPECT_CALL(*(this->request_timer), expires_from_now(_)).Times(Exactly(1));
        this->build_failure_detector();

        this->failure_detector->request_seen(req_a);
        this->request_timer_callback(boost::system::error_code());
    }

    TEST_F(pbft_failure_detector_test, add_completed_request_hash_must_update_completed_requests_and_completed_request_queue)
    {
        this->build_failure_detector();

        EXPECT_TRUE( this->failure_detector->completed_requests.empty());
        EXPECT_TRUE( this->failure_detector->completed_request_queue.empty());

        this->failure_detector->add_completed_request_hash("request_hash_000");

        EXPECT_EQ( size_t(1), this->failure_detector->completed_requests.size());
        EXPECT_EQ( size_t(1), this->failure_detector->completed_request_queue.size());
    }

    TEST_F(pbft_failure_detector_test, add_completed_request_hash_must_garbage_collect)
    {
        this->build_failure_detector();

        EXPECT_TRUE( this->failure_detector->completed_requests.empty());
        EXPECT_TRUE( this->failure_detector->completed_request_queue.empty());

        // load up the completed request hash containers
        std::stringstream hash;
        for(size_t i=0; i<bzn::max_completed_requests_memory; ++i)
        {
            hash.str("");
            hash << "hash_" << i;
            this->failure_detector->add_completed_request_hash(hash.str());
        }
        EXPECT_EQ( size_t(bzn::max_completed_requests_memory), this->failure_detector->completed_requests.size());
        EXPECT_EQ( size_t(bzn::max_completed_requests_memory), this->failure_detector->completed_request_queue.size());

        EXPECT_FALSE(this->failure_detector->completed_requests.end() == this->failure_detector->completed_requests.find("hash_0"));
        EXPECT_EQ("hash_0", this->failure_detector->completed_request_queue.front());

        // Garbage collection should start to take effect
        this->failure_detector->add_completed_request_hash("extra_hash_000");

        EXPECT_EQ( size_t(bzn::max_completed_requests_memory), this->failure_detector->completed_requests.size());
        EXPECT_EQ( size_t(bzn::max_completed_requests_memory), this->failure_detector->completed_request_queue.size());
        EXPECT_TRUE(this->failure_detector->completed_requests.end() == this->failure_detector->completed_requests.find("hash_0"));
        EXPECT_EQ("hash_1", this->failure_detector->completed_request_queue.front());

        this->failure_detector->add_completed_request_hash("extra_hash_001");
        this->failure_detector->add_completed_request_hash("extra_hash_002");
        this->failure_detector->add_completed_request_hash("extra_hash_003");
        this->failure_detector->add_completed_request_hash("extra_hash_004");

        EXPECT_EQ( size_t(bzn::max_completed_requests_memory), this->failure_detector->completed_requests.size());
        EXPECT_EQ( size_t(bzn::max_completed_requests_memory), this->failure_detector->completed_request_queue.size());
        EXPECT_TRUE(this->failure_detector->completed_requests.end() == this->failure_detector->completed_requests.find("hash_4"));
        EXPECT_EQ("hash_5", this->failure_detector->completed_request_queue.front());
    }

}

