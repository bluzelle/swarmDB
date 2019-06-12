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

#include <monitor/monitor.hpp>
#include <mocks/mock_system_clock.hpp>
#include <include/boost_asio_beast.hpp>
#include <mocks/mock_system_clock.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <options/options.hpp>
#include <options/simple_options.hpp>

#include <regex>
#include <gmock/gmock.h>
#include <mocks/mock_options_base.hpp>

using namespace ::testing;

class monitor_test : public Test
{
public:
    std::shared_ptr<bzn::asio::mock_io_context_base> io_context = std::make_shared<bzn::asio::mock_io_context_base>();
    std::shared_ptr<bzn::mock_system_clock> clock = std::make_shared<bzn::mock_system_clock>();
    std::shared_ptr<bzn::mock_options_base> options = std::make_shared<bzn::mock_options_base>();
    std::shared_ptr<bzn::monitor> monitor;

    uint64_t current_time = 0;
    std::vector<std::string> sent_messages;

    boost::asio::ip::udp::endpoint ep;
    bool send_works = true;

    bzn::simple_options soptions;

    monitor_test()
    {
        EXPECT_CALL(*(this->options), get_uuid()).WillRepeatedly(Return("uuid"));
        EXPECT_CALL(*(this->options), get_monitor_endpoint(_)).WillRepeatedly(Invoke([&](auto){return this->ep;}));
        EXPECT_CALL(*(this->options), get_stack()).WillRepeatedly(Return("utest"));
        EXPECT_CALL(*(this->options), get_swarm_id()).WillRepeatedly(Return("0"));
        this->soptions.set(bzn::option_names::MONITOR_MAX_TIMERS, "100");
        this->soptions.set(bzn::option_names::MONITOR_COLLATE, "false");
        EXPECT_CALL(*(this->options), get_simple_options()).WillRepeatedly(Invoke(
                [&]() -> const bzn::simple_options&
                {
                    return this->soptions;
                }
                ));

        EXPECT_CALL(*(this->clock), microseconds_since_epoch()).WillRepeatedly(Invoke(
                [&]()
                {
                    return this->current_time;
                }
        ));

        EXPECT_CALL(*(this->io_context), make_unique_udp_socket()).WillRepeatedly(Invoke(
                [&]()
                {
                    auto socket = std::make_unique<bzn::asio::mock_udp_socket_base>();
                    EXPECT_CALL(*socket, async_send_to(_, _, _)).WillRepeatedly(Invoke(
                            [&](auto buffer, auto /*endpoint*/, auto handler)
                            {
                                sent_messages.emplace_back(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                                if(this->send_works)
                                {
                                    handler(boost::system::error_code{}, buffer.size());
                                }
                                else
                                {
                                    handler(boost::asio::error::connection_refused, 0);
                                }
                            }
                            ));
                    return socket;
                }
                ));

        this->monitor = std::make_shared<bzn::monitor>(this->options, this->io_context, this->clock);
    }

    std::pair<std::string, uint64_t> parse_counter(const std::string& message)
    {
        std::regex counter_regex("^(.*):(\\d*)\\|(.*)$");
        std::smatch result;

        EXPECT_TRUE(std::regex_match(message, result, counter_regex));
        EXPECT_EQ(result[3].str(), "c");

        return std::pair(result[1].str(), std::stoull(result[2].str()));
    };

    std::pair<std::string, uint64_t> parse_timer(const std::string& message)
    {
        std::regex timer_regex("^(.*):(\\d*)\\|(.*)$");
        std::smatch result;

        EXPECT_TRUE(std::regex_match(message, result, timer_regex));
        EXPECT_EQ(result[3].str(), "us");

        return std::pair(result[1].str(), std::stoull(result[2].str()));
    };
};

TEST_F(monitor_test, test_that_counters_emit_metric)
{
    this->monitor->send_counter(bzn::statistic::message_sent);
    this->monitor->send_counter(bzn::statistic::message_sent_bytes, 15u);

    EXPECT_EQ(this->parse_counter(this->sent_messages.at(0)).second, 1u);
    EXPECT_EQ(this->parse_counter(this->sent_messages.at(1)).second, 15u);
}

TEST_F(monitor_test, test_that_timers_emit_metric)
{
    this->monitor->start_timer("hash");
    this->current_time = 10;
    this->monitor->finish_timer(bzn::statistic::request_latency, "hash");

    auto res = this->parse_timer(this->sent_messages.at(0));

    EXPECT_EQ(res.second, 10u);
}

TEST_F(monitor_test, test_concurrent_timers)
{
    this->current_time = 10;
    this->monitor->start_timer("A");
    this->current_time = 20;
    this->monitor->start_timer("B");

    this->current_time = 100;
    this->monitor->finish_timer(bzn::statistic::request_latency, "A");
    this->current_time = 1000;
    this->monitor->finish_timer(bzn::statistic::request_latency, "B");

    EXPECT_EQ(this->parse_timer(this->sent_messages.at(0)).second, 100u - 10u);
    EXPECT_EQ(this->parse_timer(this->sent_messages.at(1)).second, 1000u - 20u);
}

TEST_F(monitor_test, test_timer_with_duplicate_triggers)
{
    this->current_time = 10;
    this->monitor->start_timer("A");
    this->current_time = 21;
    this->monitor->start_timer("A");

    this->current_time = 30;
    this->monitor->finish_timer(bzn::statistic::request_latency, "A");
    this->current_time = 43;
    this->monitor->finish_timer(bzn::statistic::request_latency, "A");

    EXPECT_EQ(this->parse_timer(this->sent_messages.at(0)).second, 30u - 10u);
    EXPECT_EQ(this->sent_messages.size(), 1u);
}

TEST_F(monitor_test, test_no_endpoint)
{
    auto options2 = std::make_shared<NiceMock<bzn::mock_options_base>>();
    EXPECT_CALL(*options2, get_monitor_endpoint(_)).WillRepeatedly(Return(std::nullopt));
    auto monitor2 = std::make_shared<bzn::monitor>(options2, io_context, clock);

    
    monitor2->send_counter(bzn::statistic::message_sent);
    monitor2->start_timer("a");
    monitor2->finish_timer(bzn::statistic::request_latency, "a");

    EXPECT_EQ(this->sent_messages.size(), 0u);
}

TEST_F(monitor_test, test_timers_cleanup)
{
    for(int i=0; i<200; /*more than we remember at once*/ i++)
    {
        this->monitor->start_timer(std::to_string(i));
    }

    this->monitor->finish_timer(bzn::statistic::request_latency, std::to_string(0));

    EXPECT_EQ(this->sent_messages.size(), 0u);
}

TEST_F(monitor_test, test_send_fails)
{
    this->send_works = false;
    monitor->send_counter(bzn::statistic::message_sent);
    monitor->start_timer("a");
    monitor->finish_timer(bzn::statistic::request_latency, "a");

    // just testing for no crash
}

TEST_F(monitor_test, test_collate_no_messages_sent_before_time_passes)
{
    this->soptions.set(bzn::option_names::MONITOR_COLLATE, "true");
    this->soptions.set(bzn::option_names::MONITOR_COLLATE_INTERVAL_SECONDS, "1");

    monitor->send_counter(bzn::statistic::message_sent);

    EXPECT_EQ(this->sent_messages.size(), 0u);
}

TEST_F(monitor_test, test_collate_all_messages_sent)
{
    this->soptions.set(bzn::option_names::MONITOR_COLLATE, "true");
    this->soptions.set(bzn::option_names::MONITOR_COLLATE_INTERVAL_SECONDS, "1");

    // note that these quantities are chosen such that the sums of any two distinct subsets are distinct; this
    // is required to make the way we're examining the results work correctly.
    monitor->send_counter(bzn::statistic::message_sent, 3);
    monitor->send_counter(bzn::statistic::message_sent, 20);
    monitor->send_counter(bzn::statistic::message_sent_bytes, 100);

    EXPECT_EQ(this->sent_messages.size(), 0u);

    this->current_time = 2000000;
    monitor->send_counter(bzn::statistic::hash_computed, 4000);

    std::unordered_map<uint64_t, std::string> result;
    for(const auto& msg : this->sent_messages)
    {
        auto pair = this->parse_counter(msg);
        result.insert(std::make_pair(pair.second, pair.first));
    }

    EXPECT_EQ(this->sent_messages.size(), 3u);
    EXPECT_EQ(result.size(), 3u);

    EXPECT_NE(result.find(23), result.end());
    EXPECT_NE(result.at(23).find("sent"), std::string::npos);
    EXPECT_EQ(result.at(23).find("bytes"), std::string::npos);

    EXPECT_NE(result.find(100), result.end());
    EXPECT_NE(result.at(100).find("sent"), std::string::npos);
    EXPECT_NE(result.at(100).find("bytes"), std::string::npos);

    EXPECT_NE(result.find(4000), result.end());
    EXPECT_NE(result.at(4000).find("hash"), std::string::npos);

}
