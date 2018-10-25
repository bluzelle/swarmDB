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

namespace
{

    class pbft_failure_detector_test : public Test
    {
    public:
        std::shared_ptr<bzn::pbft_failure_detector> failure_detector;

        std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context =
                std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>();
        std::unique_ptr<bzn::asio::Mocksteady_timer_base> request_timer =
                std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

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

            //req_a.set_operation("do something");
            //req_b.set_operation("do something else");

            req_a.set_client("alice");
            req_b.set_client("bob");

            req_a.set_timestamp(1);
            req_b.set_timestamp(2);
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

    TEST_F(pbft_failure_detector_test, timeout_restarts_timer)
    {
        EXPECT_CALL(*(this->request_timer), expires_from_now(_)).Times(Exactly(2));
        this->build_failure_detector();

        this->failure_detector->request_seen(req_a);
        this->request_timer_callback(boost::system::error_code());
    }

}

