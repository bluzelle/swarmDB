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

#include <status/status.hpp>
#include <swarm_version.hpp>

using namespace bzn;

namespace
{
    const std::string NAME_KEY{"name"};
    const std::string STATUS_KEY{"status"};
    const std::string STATUS_MSG{"status"};
    const std::string VERSION_KEY{"version"};
    const std::string MODULE_KEY{"module"};
}


status::status(std::shared_ptr<bzn::node_base> node, bzn::status::status_provider_list_t&& status_providers)
    : node(std::move(node))
    , status_providers(std::move(status_providers))
{
}


void
status::start()
{
    std::call_once(this->start_once,
        [this]()
        {
            if (!this->node->register_for_message(STATUS_MSG,
                std::bind(&status::handle_ws_status_messages, shared_from_this(), std::placeholders::_1, std::placeholders::_2)))
            {
                throw std::runtime_error("Unable to register for STATUS messages!");
            }
        });
}


void
status::handle_ws_status_messages(const bzn::message& ws_msg, std::shared_ptr<bzn::session_base> session)
{
    auto response_msg = std::make_shared<bzn::message>(ws_msg);

    (*response_msg)[VERSION_KEY] = SWARM_VERSION;
    (*response_msg)[MODULE_KEY] = bzn::message();

    for (const auto& provider : this->status_providers)
    {
        if (auto provider_shared_ptr = provider.lock())
        {
            bzn::message entry;

            entry[NAME_KEY] = provider_shared_ptr->get_name();
            entry[STATUS_KEY] = provider_shared_ptr->get_status();

            (*response_msg)[MODULE_KEY].append(entry);
        }
    }

    LOG(debug) << response_msg->toStyledString().substr(0, MAX_MESSAGE_SIZE);

    session->send_message(response_msg, false);
}
