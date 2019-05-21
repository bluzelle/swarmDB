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

#include <chaos/chaos.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <gtest/gtest.h>
#include <options/options.hpp>

using namespace ::testing;

class chaos_test : public Test
{
public:
    std::shared_ptr<bzn::options> options;
    std::shared_ptr<bzn::asio::mock_io_context_base> mock_io_context = std::make_shared<NiceMock<bzn::asio::mock_io_context_base>>();

    std::unique_ptr<bzn::asio::mock_steady_timer_base> node_crash_timer =
            std::make_unique<NiceMock<bzn::asio::mock_steady_timer_base>>();
    std::unique_ptr<bzn::asio::mock_steady_timer_base> second_timer =
            std::make_unique<NiceMock<bzn::asio::mock_steady_timer_base>>();

    bzn::asio::wait_handler node_crash_handler;
    bzn::asio::wait_handler second_timer_handler;

    std::shared_ptr<bzn::chaos> chaos;

    size_t timer_waited = 0;

    // This pattern copied from audit test
    chaos_test()
    {
        EXPECT_CALL(*(this->mock_io_context), make_unique_steady_timer())
                .Times(AnyNumber())
                .WillOnce(Invoke(
                        [&](){return std::move(this->node_crash_timer);}
                )).WillOnce(Invoke(
                        [&](){return std::move(this->second_timer);}
                ));

        EXPECT_CALL(*(this->node_crash_timer), async_wait(_))
                .Times(AnyNumber())
                .WillRepeatedly(Invoke(
                        [&](auto handler)
                        {
                            this->node_crash_handler = handler;
                            this->timer_waited++;
                        }
                ));

        EXPECT_CALL(*(this->second_timer), async_wait(_))
                .Times(AnyNumber())
                .WillRepeatedly(Invoke(
                        [&](auto handler)
                        {
                            this->second_timer_handler = handler;
                        }
                ));

        this->options = std::make_shared<bzn::options>();

        this->options->get_mutable_simple_options().set(bzn::option_names::CHAOS_ENABLED, "true");
        this->options->get_mutable_simple_options().set(bzn::option_names::CHAOS_MESSAGE_DELAY_CHANCE, "0.5");
        this->options->get_mutable_simple_options().set(bzn::option_names::CHAOS_MESSAGE_DROP_CHANCE, "0.5");
    }

    void build_chaos()
    {
        this->chaos = std::make_shared<bzn::chaos>(this->mock_io_context, this->options);
        this->chaos->start();
    }

};


using chaos_test_DeathTest = chaos_test; // https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#death-test-naming
TEST_F(chaos_test_DeathTest, test_crash_scheduled_and_executed)
{
    this->build_chaos();
    EXPECT_EQ(this->timer_waited, 1u);

    ASSERT_DEATH(this->node_crash_handler(boost::system::error_code()), "");

}

TEST_F(chaos_test, test_messages_never_dropped_or_delayed_when_disabled)
{
    this->build_chaos();
    this->options->get_mutable_simple_options().set(bzn::option_names::CHAOS_ENABLED, "false");

    for (int i=0; i<10000; i++)
    {
        ASSERT_FALSE(this->chaos->is_message_dropped());
        ASSERT_FALSE(this->chaos->is_message_delayed());
    }
}

TEST_F(chaos_test, test_messages_sometimes_dropped_or_delayed_sometimes_not)
{
    this->build_chaos();
    uint dropped_count = 0;
    uint delayed_count = 0;
    uint not_dropped_count = 0;
    uint not_delayed_count = 0;

    for (int i=0; i<10000; i++)
    {
        if (this->chaos->is_message_dropped())
        {
            dropped_count ++;
        }
        else
        {
            not_dropped_count ++;
        }

        if (this->chaos->is_message_delayed())
        {
            delayed_count++;
        }
        else
        {
            not_delayed_count++;
        }

        if (dropped_count>0 && not_dropped_count>0 && delayed_count>0 && not_delayed_count>0)
        {
            return;
        }
    }

    FAIL();
}

TEST_F(chaos_test, reschedule_retries_message)
{
    this->build_chaos();
    bool called = false;
    this->chaos->reschedule_message([&](){called = true;});

    this->second_timer_handler(boost::system::error_code());

    ASSERT_TRUE(called);
}
