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
#include <swarm_git_commit.hpp>
#include <proto/status.pb.h>

using namespace bzn;

namespace
{
    const std::string NAME_KEY{"name"};
    const std::string STATUS_KEY{"status"};
    const std::string STATUS_MSG{"status"};
    const std::string VERSION_KEY{"version"};
    const std::string MODULE_KEY{"module"};
    const std::string UPTIME_KEY{"uptime"};
    const std::string COMMIT_KEY{"commit"};
    const std::string PBFT_ENABLED_KEY{"pbft_enabled"};

    std::string get_uptime(const std::chrono::steady_clock::time_point& start_time)
    {
        using namespace std::chrono;

        auto uptime = duration_cast<seconds>(steady_clock::now() - start_time);

        auto d = duration_cast<duration<int64_t, std::ratio<3600 * 24>>>(uptime);
        auto h = duration_cast<hours>(uptime -= d);
        auto m = duration_cast<minutes>(uptime -= h);

        std::stringstream ss;
        ss << d.count() << " days, " << h.count() << " hours, " << m.count() << " minutes";

        return ss.str();
    }
}


status::status(std::shared_ptr<bzn::node_base> node, bzn::status::status_provider_list_t&& status_providers, const bool pbft_enabled)
    : node(std::move(node))
    , status_providers(std::move(status_providers))
    , start_time(std::chrono::steady_clock::now())
    , pbft_enabled(pbft_enabled)
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

            if (!this->node->register_for_message(bzn_envelope::kStatusRequest,
                std::bind(&status::handle_status_request_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2)))
            {
                throw std::runtime_error("Unable to register for STATUS REQUEST messages!");
            }
        });
}


bzn::json_message
status::query_modules()
{
    Json::Value module_status;

    for (const auto& provider : this->status_providers)
    {
        if (auto provider_shared_ptr = provider.lock())
        {
            bzn::json_message entry;

            entry[NAME_KEY] = provider_shared_ptr->get_name();
            entry[STATUS_KEY] = provider_shared_ptr->get_status();

            module_status.append(entry);
        }
    }

    return module_status;
}


void
status::handle_ws_status_messages(const bzn::json_message& ws_msg, std::shared_ptr<bzn::session_base> session)
{
    auto response_msg = std::make_shared<bzn::json_message>(ws_msg);

    (*response_msg)[VERSION_KEY] = SWARM_VERSION;
    (*response_msg)[COMMIT_KEY] = SWARM_GIT_COMMIT;
    (*response_msg)[UPTIME_KEY] = get_uptime(this->start_time);
    (*response_msg)[MODULE_KEY] = this->query_modules();
    (*response_msg)[PBFT_ENABLED_KEY] = this->pbft_enabled;

    LOG(debug) << response_msg->toStyledString().substr(0, MAX_MESSAGE_SIZE);

    session->send_message(response_msg, false);
}


void
status::handle_status_request_message(const bzn_envelope& /*msg*/, std::shared_ptr<bzn::session_base> session)
{
    status_response srm;

    srm.set_swarm_version(SWARM_VERSION);
    srm.set_swarm_git_commit(SWARM_GIT_COMMIT);
    srm.set_uptime(get_uptime(this->start_time));
    srm.set_pbft_enabled(this->pbft_enabled);

    Json::Value module_status;
    module_status[MODULE_KEY] = this->query_modules();
    srm.set_module_status_json(module_status.toStyledString());

    LOG(debug) << srm.DebugString().substr(0, MAX_MESSAGE_SIZE);

    session->send_message(std::make_shared<bzn::encoded_message>(srm.SerializeAsString()), false);
}
