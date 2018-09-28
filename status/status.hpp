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

#include <include/bluzelle.hpp>
#include <node/node_base.hpp>
#include <status/status_provider_base.hpp>
#include <memory>
#include <mutex>


namespace bzn
{
    class status final : public std::enable_shared_from_this<status>
    {
    public:
        using status_provider_list_t = std::vector<std::weak_ptr<bzn::status_provider_base>>;

        status(std::shared_ptr<bzn::node_base> node, status_provider_list_t&& status_providers);

        void start();

    private:
        void handle_ws_status_messages(const bzn::json_message& ws_msg, std::shared_ptr<bzn::session_base> session);

        std::shared_ptr<bzn::node_base> node;

        status_provider_list_t status_providers;
        std::once_flag start_once;
    };

} // namespace bzn
