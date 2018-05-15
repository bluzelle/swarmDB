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

#include <include/bluzelle.hpp>
#include <ethereum/ethereum.hpp>
#include <gmock/gmock.h>

using namespace ::testing;

namespace
{
    bool get_api_token(std::string& api_token)
    {
        const char* token = getenv("ETHERSCAN_IO_API_TOKEN");

        if (token)
        {
            api_token = token;
            return true;
        }

        LOG(error) << "************************************************************";
        LOG(error) << "*** MISSING ETHERSCAN_IO_API_TOKEN ENVIRONMENT VARIABLE! ***";
        LOG(error) << "************************************************************";

        return false;
    }
}


// env ETHERSCAN_IO_API_TOKEN=<API_TOKEN> ./ethereum_tests
namespace  bzn
{
    TEST(ethereum, test_that_there_is_a_token_balance_for_a_valid_address)
    {
        // disable test in CI
        std::string api_token;
        if (get_api_token(api_token))
        {
            EXPECT_GT(bzn::ethereum().get_ether_balance("0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae", api_token), double(0));
        }
    }


    TEST(ethereum, test_that_there_is_no_balance_for_an_invalid_address)
    {
        // disable test in CI
        std::string api_token;
        if (get_api_token(api_token))
        {

            EXPECT_EQ(bzn::ethereum().get_ether_balance("0xde0b295669aZCZCXCZCZCZCZCZCXC40f4cb697bae", api_token), double(0));
        }
    }

} // namespace bzn
