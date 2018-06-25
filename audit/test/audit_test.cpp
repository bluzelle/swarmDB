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

#include <audit/audit.hpp>
#include <mocks/mock_node_base.hpp>
#include <mocks/mock_node_base.hpp>
#include <mocks/mock_boost_asio_beast.hpp>
#include <boost/range/irange.hpp>

using namespace ::testing;

class audit_test : public Test
{
public:
    std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context = std::make_shared<bzn::asio::Mockio_context_base>();
    std::shared_ptr<bzn::Mocknode_base> mock_node = std::make_shared<bzn::Mocknode_base>();

    std::unique_ptr<bzn::asio::Mocksteady_timer_base> leader_alive_timer =
            std::make_unique<bzn::asio::Mocksteady_timer_base>();
    std::unique_ptr<bzn::asio::Mocksteady_timer_base> leader_progress_timer =
            std::make_unique<bzn::asio::Mocksteady_timer_base>();

    bzn::asio::wait_handler leader_alive_timer_callback;
    bzn::asio::wait_handler leader_progress_timer_callback;

    std::shared_ptr<bzn::audit> audit;

    audit_test()
    {
        // Here we are depending on the fact that audit.cpp will initialize the leader alive timer before the leader
        // progress timer. This is ugly, but it seems better than reaching inside audit to get the private timers or
        // call the private callbacks.
        EXPECT_CALL(*(this->mock_io_context), make_unique_steady_timer())
                .WillOnce(Invoke(
                [&](){return std::move(this->leader_alive_timer);}
        ))
                .WillOnce(Invoke(
                [&](){return std::move(this->leader_progress_timer);}
        ));

        EXPECT_CALL(*(this->leader_alive_timer), async_wait(_))
                .Times(AtLeast(1)) // This enforces the assumption: if this actually the leader progress timer,
                                   // then there are some tests that will never wait on it
                .WillRepeatedly(Invoke(
                [&](auto handler){this->leader_alive_timer_callback = handler;}
        ));
        EXPECT_CALL(*(this->leader_progress_timer), async_wait(_))
                .Times(AnyNumber())
                .WillRepeatedly(Invoke(
                [&](auto handler){this->leader_progress_timer_callback = handler;}
        ));
    }

    void build_audit()
    {
        // We cannot construct this during our constructor because doing so invalidates our timer pointers,
        // which prevents tests from setting expectations on them
        this->audit = std::make_shared<bzn::audit>(this->mock_io_context, this->mock_node);
        this->audit->start();
    }

    bool progress_timer_running;
    uint progress_timer_reset_count;

    void progress_timer_status_expectations()
    {
        this->progress_timer_running = false;
        this->progress_timer_reset_count = 0;

        EXPECT_CALL(*(this->leader_progress_timer), cancel()).WillRepeatedly(Invoke(
                [&]()
                {
                    this->progress_timer_running = false;
                }
        ));

        EXPECT_CALL(*(this->leader_progress_timer), expires_from_now(_)).WillRepeatedly(Invoke(
                [&](auto /*time*/)
                {
                    this->progress_timer_running = true;
                    this->progress_timer_reset_count++;
                    return 0;
                }
        ));
    }

};

TEST_F(audit_test, test_timers_constructed_correctly)
{
    // This just tests the expecations built into the superclass
    this->build_audit();
}

TEST_F(audit_test, no_errors_initially)
{
    this->build_audit();
    EXPECT_EQ(this->audit->error_count(), 0u);
    EXPECT_EQ(this->audit->error_strings().size(), 0u);
}

TEST_F(audit_test, audit_throws_error_when_leaders_conflict)
{
    this->build_audit();
    leader_status a, b, c;

    a.set_leader("fred");
    a.set_term(1);

    b.set_leader("smith");
    b.set_term(2);

    c.set_leader("francheskitoria");
    c.set_term(1);

    this->audit->handle_leader_status(a);
    this->audit->handle_leader_status(b);
    this->audit->handle_leader_status(c);
    this->audit->handle_leader_status(b);

    EXPECT_EQ(this->audit->error_count(), 1u);
}

TEST_F(audit_test, audit_throws_error_when_commits_conflict)
{
    this->build_audit();
    commit_notification a, b, c;

    a.set_operation("do something");
    a.set_log_index(1);

    b.set_operation("do a different thing");
    b.set_log_index(2);

    c.set_operation("do something else");
    c.set_log_index(1);

    this->audit->handle_commit(a);
    this->audit->handle_commit(b);
    this->audit->handle_commit(c);
    this->audit->handle_commit(b);

    EXPECT_EQ(this->audit->error_count(), 1u);
}


TEST_F(audit_test, audit_throws_error_when_no_leader_alive)
{
    EXPECT_CALL(*(this->leader_alive_timer), expires_from_now(_)).Times(AtLeast(1));
    this->build_audit();

    this->leader_alive_timer_callback(boost::system::error_code());
    EXPECT_EQ(this->audit->error_count(), 1u);
}

TEST_F(audit_test, audit_throws_error_when_leader_stuck)
{
    EXPECT_CALL(*(this->leader_progress_timer), expires_from_now(_)).Times(AtLeast(1));
    this->build_audit();

    leader_status ls;
    ls.set_leader("fred");
    ls.set_current_commit_index(6);
    ls.set_current_log_index(8);

    this->audit->handle_leader_status(ls);
    this->audit->handle_leader_status(ls);

    this->leader_progress_timer_callback(boost::system::error_code());
    EXPECT_EQ(this->audit->error_count(), 1u);
}

TEST_F(audit_test, audit_resets_leader_stuck_on_message)
{
    bool reset;
    EXPECT_CALL(*(this->leader_progress_timer), cancel()).WillRepeatedly(Invoke(
            [&](){reset = true;}
            ));

    this->build_audit();

    leader_status ls1;
    ls1.set_leader("fred");
    ls1.set_current_commit_index(6);
    ls1.set_current_log_index(8);

    leader_status ls2 = ls1;
    ls2.set_current_commit_index(7);

    this->audit->handle_leader_status(ls1);
    reset = false;
    this->audit->handle_leader_status(ls2);

    EXPECT_TRUE(reset);
    EXPECT_EQ(this->audit->error_count(), 0u);

    this->leader_progress_timer_callback(boost::system::error_code());

    EXPECT_EQ(this->audit->error_count(), 1u);
}

TEST_F(audit_test, audit_resets_leader_alive_on_message)
{
    bool reset;
    EXPECT_CALL(*(this->leader_alive_timer), cancel()).WillRepeatedly(Invoke(
            [&](){reset = true;}
    ));

    this->build_audit();

    leader_status ls1;
    ls1.set_leader("fred");
    ls1.set_current_commit_index(6);
    ls1.set_current_log_index(8);

    reset = false;
    this->audit->handle_leader_status(ls1);
    EXPECT_TRUE(reset);
    EXPECT_EQ(this->audit->error_count(), 0u);

    this->leader_alive_timer_callback(boost::system::error_code());

    EXPECT_EQ(this->audit->error_count(), 1u);

}

TEST_F(audit_test, audit_no_error_or_timer_when_leader_idle)
{
    this->progress_timer_status_expectations();
    this->build_audit();

    leader_status ls1;
    ls1.set_leader("fred");
    ls1.set_current_commit_index(6);
    ls1.set_current_log_index(8);

    leader_status ls2 = ls1;
    ls2.set_current_commit_index(8);

    EXPECT_FALSE(this->progress_timer_running);
    this->audit->handle_leader_status(ls1);
    EXPECT_TRUE(this->progress_timer_running);
    this->audit->handle_leader_status(ls2);
    EXPECT_FALSE(this->progress_timer_running);

    EXPECT_EQ(this->audit->error_count(), 0u);
}

TEST_F(audit_test, audit_no_error_when_leader_livelock)
{
    // The case here is when new leaders keep getting elected, but none of them can make any progress
    // We would like to throw an error here, but detecting it properly is complex
    this->progress_timer_status_expectations();
    this->build_audit();

    leader_status ls1;
    ls1.set_leader("fred");
    ls1.set_current_commit_index(6);
    ls1.set_current_log_index(8);

    leader_status ls2;
    ls2.set_leader("joe");
    ls2.set_term(ls1.term()+1);
    ls2.set_current_commit_index(9);
    ls2.set_current_log_index(12);

    EXPECT_FALSE(this->progress_timer_running);

    this->audit->handle_leader_status(ls1);
    EXPECT_TRUE(this->progress_timer_running);

    uint old_count = this->progress_timer_reset_count;
    this->audit->handle_leader_status(ls2);
    EXPECT_TRUE(this->progress_timer_running);
    EXPECT_GT(this->progress_timer_reset_count, old_count);

    old_count = this->progress_timer_reset_count;
    ls1.set_term(ls2.term()+1);
    this->audit->handle_leader_status(ls1);
    EXPECT_TRUE(this->progress_timer_running);
    EXPECT_GT(this->progress_timer_reset_count, old_count);

    EXPECT_EQ(this->audit->error_count(), 0u);
}

TEST_F(audit_test, audit_no_error_when_leader_progress)
{
    this->progress_timer_status_expectations();
    this->build_audit();

    EXPECT_FALSE(this->progress_timer_running);

    leader_status ls1;
    ls1.set_leader("joe");

    uint old_count = this->progress_timer_reset_count;
    for(auto i : boost::irange(0, 5))
    {
        ls1.set_current_commit_index(i);
        ls1.set_current_log_index(i+2);

        this->audit->handle_leader_status(ls1);

        EXPECT_TRUE(this->progress_timer_running);
        EXPECT_GT(this->progress_timer_reset_count, old_count);

        old_count = this->progress_timer_reset_count;
    }

    EXPECT_EQ(this->audit->error_count(), 0u);
}

TEST_F(audit_test, audit_throws_error_when_leader_alive_then_stuck)
{
    this->progress_timer_status_expectations();
    this->build_audit();

    leader_status ls1;
    ls1.set_leader("joe");
    ls1.set_current_commit_index(5);
    ls1.set_current_log_index(5);

    leader_status ls2 = ls1;
    ls2.set_current_log_index(17);

    this->audit->handle_leader_status(ls1);
    EXPECT_FALSE(this->progress_timer_running);
    this->audit->handle_leader_status(ls2);
    EXPECT_TRUE(this->progress_timer_running);
    this->leader_progress_timer_callback(boost::system::error_code());

    EXPECT_EQ(this->audit->error_count(), 1u);
}

TEST_F(audit_test, audit_throws_error_when_leader_switch_then_stuck)
{
    this->build_audit();

    leader_status ls1;
    ls1.set_leader("joe");
    ls1.set_current_commit_index(5);
    ls1.set_current_log_index(5);

    leader_status ls2 = ls1;
    ls2.set_leader("freddie");
    ls2.set_term(ls1.term() + 1);

    this->audit->handle_leader_status(ls1);
    this->audit->handle_leader_status(ls2);
    EXPECT_EQ(this->audit->error_count(), 0u);

    this->leader_alive_timer_callback(boost::system::error_code());
    EXPECT_EQ(this->audit->error_count(), 1u);
}

