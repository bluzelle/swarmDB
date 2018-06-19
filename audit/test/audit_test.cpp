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

using namespace ::testing;

TEST(audit_test, no_errors_initially){
    bzn::audit a(nullptr);
    EXPECT_EQ(a.error_count(), 0u);
    EXPECT_EQ(a.error_strings().size(), 0u);
}

TEST(audit_test, conflicting_leaders)
{
    leader_status a, b, c;

    a.set_leader("fred");
    a.set_term(1);

    b.set_leader("smith");
    b.set_term(2);

    c.set_leader("francheskitoria");
    c.set_term(1);

    bzn::audit audit(nullptr);

    audit.handle_leader_status(a);
    audit.handle_leader_status(b);
    audit.handle_leader_status(c);
    audit.handle_leader_status(b);

    EXPECT_EQ(audit.error_count(), 1u);
}

TEST(audit_test, conflicting_commits)
{
    commit_notification a, b, c;

    a.set_operation("do something");
    a.set_log_index(1);

    b.set_operation("do a different thing");
    b.set_log_index(2);

    c.set_operation("do something else");
    c.set_log_index(1);

    bzn::audit audit(nullptr);

    audit.handle_commit(a);
    audit.handle_commit(b);
    audit.handle_commit(c);
    audit.handle_commit(b);

    EXPECT_EQ(audit.error_count(), 1u);
}
