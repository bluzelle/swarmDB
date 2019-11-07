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

#include <mocks/smart_mock_node.hpp>


using namespace ::testing;

bzn::smart_mock_node::smart_mock_node()
{
    EXPECT_CALL(*this, register_for_message(_, _)).WillRepeatedly(Invoke(
            [&](auto type, auto handler)
            {
                if (this->registrants.count(type) > 0)
                {
                    throw std::runtime_error("duplicate node registration");
                }

                this->registrants[type] = handler;
                return true;
            }
            ));

    EXPECT_CALL(*this, multicast_maybe_signed_message(_, _)).WillRepeatedly(Invoke(
            [&](auto endpoints, auto message)
            {
                for (const auto ep : *endpoints)
                {
                    this->send_maybe_signed_message(ep, message);
                }
            }));
}

void bzn::smart_mock_node::deliver(const bzn_envelope& msg)
{
    if (this->registrants.count(msg.payload_case()) == 0)
    {
        throw std::runtime_error("undeliverable message");
    }

    this->registrants.at(msg.payload_case())(msg, nullptr);
}

void bzn::smart_mock_node::clear()
{
    this->registrants.clear();
}
