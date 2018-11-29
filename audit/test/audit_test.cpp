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

using namespace ::testing;

class audit_test : public Test
{
public:
    std::shared_ptr<bzn::asio::Mockio_context_base> mock_io_context = std::make_shared<NiceMock<bzn::asio::Mockio_context_base>>();
    std::shared_ptr<bzn::Mocknode_base> mock_node = std::make_shared<NiceMock<bzn::Mocknode_base>>();

    std::unique_ptr<bzn::asio::Mocksteady_timer_base> primary_alive_timer =
            std::make_unique<NiceMock<bzn::asio::Mocksteady_timer_base>>();

    bzn::asio::wait_handler primary_alive_timer_callback;

    std::optional<boost::asio::ip::udp::endpoint> endpoint;
    std::unique_ptr<bzn::asio::Mockudp_socket_base> socket = std::make_unique<NiceMock<bzn::asio::Mockudp_socket_base>>();

    std::shared_ptr<bzn::audit> audit;

    size_t mem_size = 1000;

    bool use_pbft = false;

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

        EXPECT_CALL(*(this->mock_io_context), make_unique_udp_socket())
                .WillOnce(Invoke(
                [&](){return std::move(this->socket);}
        ));
    }

    void build_audit()
    {
        // We cannot construct this during our constructor because doing so invalidates our timer pointers,
        // which prevents tests from setting expectations on them
        this->audit = std::make_shared<bzn::audit>(this->mock_io_context, this->mock_node, this->endpoint, "audit_test_uuid", this->mem_size);
        this->audit->start();
    }
};

TEST_F(audit_test, no_errors_initially)
{
    this->build_audit();
    EXPECT_EQ(this->audit->error_count(), 0u);
    EXPECT_EQ(this->audit->error_strings().size(), 0u);
}

TEST_F(audit_test, audit_throws_error_when_primaries_conflict)
{
    this->use_pbft = true;
    this->build_audit();
    primary_status a, b, c;

    a.set_primary("fred");
    a.set_view(1);

    b.set_primary("smith");
    b.set_view(2);

    c.set_primary("francheskitoria");
    c.set_view(1);

    this->audit->handle_primary_status(a);
    this->audit->handle_primary_status(b);
    this->audit->handle_primary_status(c);
    this->audit->handle_primary_status(b);

    EXPECT_EQ(this->audit->error_count(), 1u);
}

TEST_F(audit_test, audit_throws_error_when_pbft_commits_conflict)
{
    this->use_pbft = true;
    this->build_audit();

    pbft_commit_notification a, b, c;

    a.set_operation("do something");
    a.set_sequence_number(1);

    b.set_operation("do a different thing");
    b.set_sequence_number(2);

    c.set_operation("do something else");
    c.set_sequence_number(1);

    this->audit->handle_pbft_commit(a);
    this->audit->handle_pbft_commit(b);
    this->audit->handle_pbft_commit(c);
    this->audit->handle_pbft_commit(b);

    EXPECT_EQ(this->audit->error_count(), 1u);
}

TEST_F(audit_test, audit_throws_error_when_no_primary_alive)
{
    this->use_pbft = true;
    EXPECT_CALL(*(this->primary_alive_timer), expires_from_now(_)).Times(AtLeast(1));
    this->build_audit();

    this->primary_alive_timer_callback(boost::system::error_code());
    EXPECT_EQ(this->audit->error_count(), 1u);
}

TEST_F(audit_test, audit_resets_primary_alive_on_message)
{
    bool reset;
    EXPECT_CALL(*(this->primary_alive_timer), cancel()).WillRepeatedly(Invoke(
            [&](){reset = true;}
    ));

    this->use_pbft = true;
    this->build_audit();

    primary_status ls1;
    ls1.set_primary("fred");

    reset = false;
    this->audit->handle_primary_status(ls1);
    EXPECT_TRUE(reset);
    EXPECT_EQ(this->audit->error_count(), 0u);

    this->primary_alive_timer_callback(boost::system::error_code());

    EXPECT_EQ(this->audit->error_count(), 1u);

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

    // It's allowed to have mem size each of commits, primarys, and errors
    EXPECT_LE(this->audit->current_memory_size(), 3*this->mem_size);
}

TEST_F(audit_test, audit_still_detects_new_errors_after_forgetting_old_data)
{
    this->mem_size = 10;
    this->build_audit();

    primary_status ls1;
    ls1.set_primary("joe");

    pbft_commit_notification com;
    com.set_operation("do exciting things and stuff");

    for(auto i : boost::irange(0, 100))
    {
        ls1.set_view(i);
        com.set_sequence_number(i);

        this->audit->handle_primary_status(ls1);
        this->audit->handle_pbft_commit(com);
    }

    EXPECT_EQ(this->audit->error_count(), 0u);

    ls1.set_primary("not joe");
    com.set_operation("don't do anything");

    this->audit->handle_primary_status(ls1);
    this->audit->handle_pbft_commit(com);

    EXPECT_EQ(this->audit->error_count(), 2u);
}

TEST_F(audit_test, audit_still_counts_errors_after_forgetting_their_data)
{
    this->mem_size = 10;
    this->build_audit();

    primary_status ls1;
    ls1.set_primary("joe");

    primary_status ls2;
    ls2.set_primary("alfred");

    for(auto i : boost::irange(0, 100))
    {
        // Trigger an error, a primary elected, and a commit notification every iteration
        ls1.set_view(i);
        ls2.set_view(i);

        this->audit->handle_primary_status(ls1);
        this->audit->handle_primary_status(ls2);
    }

    EXPECT_GT(this->audit->error_count(), this->audit->error_strings().size());
    EXPECT_EQ(this->audit->error_strings().size(), this->mem_size);
}

TEST_F(audit_test, audit_sends_monitor_message_when_primary_conflict)
{
    bool error_reported = false;

    EXPECT_CALL(*(this->socket), async_send_to(_,_,_)).WillRepeatedly(Invoke([&](
         const boost::asio::const_buffer& msg,
         boost::asio::ip::udp::endpoint /*ep*/,
         std::function<void(const boost::system::error_code&, size_t)> /*handler*/)
             {
                 std::string msg_string(boost::asio::buffer_cast<const char*>(msg), msg.size());
                 if (msg_string.find(bzn::PRIMARY_CONFLICT_METRIC_NAME) != std::string::npos)
                 {
                     error_reported = true;
                 }
             }

    ));

    auto ep = boost::asio::ip::udp::endpoint{
          boost::asio::ip::address::from_string("127.0.0.1")
        , 8125
    };
    auto epopt = std::optional<boost::asio::ip::udp::endpoint>{ep};
    this->endpoint = epopt;
    this->build_audit();

    primary_status ls1;
    ls1.set_primary("joe");

    primary_status ls2 = ls1;
    ls2.set_primary("francine");

    this->audit->handle_primary_status(ls1);
    this->audit->handle_primary_status(ls2);

    EXPECT_TRUE(error_reported);
}

TEST_F(audit_test, audit_sends_monitor_message_when_commit_conflict)
{
    bool error_reported = false;

    EXPECT_CALL(*(this->socket), async_send_to(_,_,_)).WillRepeatedly(Invoke([&](
         const boost::asio::const_buffer& msg,
         boost::asio::ip::udp::endpoint /*ep*/,
         std::function<void(const boost::system::error_code&, size_t)> /*handler*/)
             {
                 std::string msg_string(boost::asio::buffer_cast<const char*>(msg), msg.size());
                 if (msg_string.find(bzn::PBFT_COMMIT_CONFLICT_METRIC_NAME) != std::string::npos)
                 {
                     error_reported = true;
                 }
             }

    ));

    auto ep = boost::asio::ip::udp::endpoint{
            boost::asio::ip::address::from_string("127.0.0.1")
            , 8125
    };
    auto epopt = std::optional<boost::asio::ip::udp::endpoint>{ep};
    this->endpoint = epopt;


    this->build_audit();

    pbft_commit_notification com1;
    com1.set_sequence_number(2);
    com1.set_operation("the first thing");

    pbft_commit_notification com2 = com1;
    com2.set_operation("the second thing");

    this->audit->handle_pbft_commit(com1);
    this->audit->handle_pbft_commit(com2);

    EXPECT_TRUE(error_reported);
}
