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

#include <options/options_base.hpp>
#include <json/json.h>


namespace bzn
{
    class options final : public bzn::options_base
    {
    public:
        bool parse_command_line(int argc, const char* argv[]);

        boost::asio::ip::tcp::endpoint get_listener() const override;

        std::string get_ethererum_address() const override;

        std::string get_bootstrap_peers_file() const override;

        std::string get_bootstrap_peers_url() const override;

        std::string get_ethererum_io_api_token() const override;

        bool get_debug_logging() const override;

        bool get_log_to_stdout() const override;

        bzn::uuid_t get_uuid() const override;

        bzn::uuid_t get_swarm_uuid() const override;

        std::chrono::seconds get_ws_idle_timeout() const override;

    private:
        bool parse(int argc, const char* argv[]);

        void load(const std::string& config_file);

        bool validate();

        Json::Value config_data;
    };

} // bzn
