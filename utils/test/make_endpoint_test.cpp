// Copyright (C) 2019 Bluzelle
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

#include <utils/make_endpoint.hpp>
#include <gtest/gtest.h>

using namespace::testing;


TEST(make_endpoint, test_test_valid_host_name_returns_correct_address)
{
    bzn::peer_address_t peer{"localhost", 80, "name", "uuid"};

    auto ep = bzn::make_endpoint(peer);

    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ((*ep).address().to_string(), "127.0.0.1");
}


TEST(make_endpoint, test_test_invalid_host_name_returns_nothing)
{
    bzn::peer_address_t peer{"localhost-asdf", 80, "name", "uuid"};

    auto ep = bzn::make_endpoint(peer);

    ASSERT_FALSE(ep.has_value());
}


TEST(make_endpoint, test_test_dotted_v4_address_returns_address)
{
    bzn::peer_address_t peer{"192.168.0.1", 80, "name", "uuid"};

    auto ep = bzn::make_endpoint(peer);

    ASSERT_TRUE(ep.has_value());
    EXPECT_EQ((*ep).address().to_string(), "192.168.0.1");
}
