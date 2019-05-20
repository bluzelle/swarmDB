// Copyright (C) 2018 Bluzelle
//
// This program is free software: you can redistribute it and/or modify
// it under the views of the GNU Affero General Public License, version 3,
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
#include <mocks/mock_boost_asio_beast.hpp>
#include <boost/range/irange.hpp>
#include <boost/asio/buffer.hpp>
#include <mocks/mock_monitor.hpp>

using namespace ::testing;

class audit_test : public Test
{
public:
    std::shared_ptr<bzn::asio::mock_io_context_base> mock_io_context = std::make_shared<NiceMock<bzn::asio::mock_io_context_base>>();
    std::shared_ptr<bzn::mock_node_base> mock_node = std::make_shared<NiceMock<bzn::mock_node_base>>();

    std::unique_ptr<bzn::asio::mock_steady_timer_base> primary_alive_timer =
            std::make_unique<NiceMock<bzn::asio::mock_steady_timer_base>>();

    bzn::asio::wait_handler primary_alive_timer_callback;

    std::shared_ptr<bzn::mock_monitor> mock_monitor = std::make_shared<StrictMock<bzn::mock_monitor>>();

    std::shared_ptr<bzn::audit> audit;

    size_t mem_size = 1000;

    audit_test()
    {
        EXPECT_CALL(*(this->mock_io_context), make_unique_steady_timer())
                .WillOnce(Invoke(
                [&](){return std::move(this->primary_alive_timer);}
        ));

        EXPECT_CALL(*(this->primary_alive_timer), async_wait(_))
                .WillRepeatedly(Invoke(
                [&](auto handler){this->primary_alive_timer_callback = handler;}
        ));
    }

    void build_audit()
    {
        // We cannot construct this during our constructor because doing so invalidates our timer pointers,
        // which prevents tests from setting expectations on them
        this->audit = std::make_shared<bzn::audit>(this->mock_io_context, this->mock_node, this->mem_size, this->mock_monitor);
        this->audit->start();
    }
};

TEST_F(audit_test, audit_throws_error_when_primaries_conflict)
{
    this->build_audit();
    primary_status a, b, c;

    a.set_primary("fred");
    a.set_view(1);

    b.set_primary("smith");
    b.set_view(2);

    c.set_primary("francheskitoria");
    c.set_view(1);

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_alive, 1)).Times(AnyNumber());
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_conflict, 1));

    this->audit->handle_primary_status(a);
    this->audit->handle_primary_status(b);
    this->audit->handle_primary_status(c);
    this->audit->handle_primary_status(b);
}

TEST_F(audit_test, audit_throws_error_when_pbft_commits_conflict)
{
    this->build_audit();

    pbft_commit_notification a, b, c;

    a.set_operation("do something");
    a.set_sequence_number(1);

    b.set_operation("do a different thing");
    b.set_sequence_number(2);

    c.set_operation("do something else");
    c.set_sequence_number(1);

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_commit, 1)).Times(AnyNumber());
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_commit_conflict, 1));

    this->audit->handle_pbft_commit(a);
    this->audit->handle_pbft_commit(b);
    this->audit->handle_pbft_commit(c);
    this->audit->handle_pbft_commit(b);
}

TEST_F(audit_test, audit_throws_error_when_no_primary_alive)
{
    EXPECT_CALL(*(this->primary_alive_timer), expires_from_now(_)).Times(AtLeast(1));
    this->build_audit();

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_no_primary, 1));

    this->primary_alive_timer_callback(boost::system::error_code());
}

TEST_F(audit_test, audit_resets_primary_alive_on_message)
{
    bool reset;
    EXPECT_CALL(*(this->primary_alive_timer), cancel()).WillRepeatedly(Invoke(
            [&](){reset = true;}
    ));

    this->build_audit();

    primary_status ls1;
    ls1.set_primary("fred");

    reset = false;
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_alive, 1)).Times(AnyNumber());
    this->audit->handle_primary_status(ls1);
    EXPECT_TRUE(reset);

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_no_primary, 1));
    this->primary_alive_timer_callback(boost::system::error_code());
}

TEST_F(audit_test, audit_forgets_old_data)
{
    this->mem_size = 10;
    this->build_audit();

    primary_status ls1;
    ls1.set_primary("joe");

    primary_status ls2;
    ls2.set_primary("alfred");

    pbft_commit_notification com;
    com.set_operation("Do some stuff!!");

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_commit, 1)).Times(AnyNumber());
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_alive, 1)).Times(AnyNumber());
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_conflict, 1)).Times(Exactly(100));

    for(auto i : boost::irange(0, 100))
    {
        // Trigger an error, a primary elected, and a commit notification every iteration
        ls1.set_view(i);
        ls2.set_view(i);
        com.set_sequence_number(i);

        this->audit->handle_primary_status(ls1);
        this->audit->handle_primary_status(ls2);
        this->audit->handle_pbft_commit(com);
    }

    // It's allowed to have mem size each of commits, primaries, and
    EXPECT_LE(this->audit->current_memory_size(), 2*this->mem_size);
}

TEST_F(audit_test, audit_still_detects_new_errors_after_forgetting_old_data)
{
    this->mem_size = 10;
    this->build_audit();

    primary_status ls1;
    ls1.set_primary("joe");

    pbft_commit_notification com;
    com.set_operation("do exciting things and stuff");

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_commit, 1)).Times(AnyNumber());
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_alive, 1)).Times(AnyNumber());

    for (auto i : boost::irange(0, 100))
    {
        ls1.set_view(i);
        com.set_sequence_number(i);

        this->audit->handle_primary_status(ls1);
        this->audit->handle_pbft_commit(com);
    }

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_commit_conflict, 1));
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_conflict, 1));
    ls1.set_primary("not joe");
    com.set_operation("don't do anything");

    this->audit->handle_primary_status(ls1);
    this->audit->handle_pbft_commit(com);
}


TEST_F(audit_test, audit_sends_monitor_message_when_primary_conflict)
{
    this->build_audit();

    primary_status ls1;
    ls1.set_primary("joe");

    primary_status ls2 = ls1;
    ls2.set_primary("francine");

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_alive, 1)).Times(AnyNumber());
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_primary_conflict, 1));

    this->audit->handle_primary_status(ls1);
    this->audit->handle_primary_status(ls2);
}

TEST_F(audit_test, audit_sends_monitor_message_when_commit_conflict)
{
    this->build_audit();

    pbft_commit_notification com1;
    com1.set_sequence_number(2);
    com1.set_operation("the first thing");

    pbft_commit_notification com2 = com1;
    com2.set_operation("the second thing");

    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_commit, 1)).Times(AnyNumber());
    EXPECT_CALL(*(this->mock_monitor), send_counter(bzn::statistic::pbft_commit_conflict, 1));

    this->audit->handle_pbft_commit(com1);
    this->audit->handle_pbft_commit(com2);
}
