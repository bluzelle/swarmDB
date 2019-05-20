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

#pragma once

#include <pbft/pbft_failure_detector_base.hpp>
#include <gmock/gmock.h>
#include <functional>

namespace bzn
{
    class mock_pbft_failure_detector_base : public pbft_failure_detector_base
    {
    public:
        MOCK_METHOD1(request_seen, void(const bzn::hash_t& req));

        MOCK_METHOD1(request_executed, void(const bzn::hash_t& req));

        MOCK_METHOD1(register_failure_handler, void(std::function<void()> handler));
    };

}
