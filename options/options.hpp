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
        const simple_options& get_simple_options() const override;

        simple_options& get_mutable_simple_options() override;

        bool parse_command_line(int argc, const char* argv[]);

        boost::asio::ip::tcp::endpoint get_listener() const override;

        std::optional<boost::asio::ip::udp::endpoint> get_monitor_endpoint(std::shared_ptr<bzn::asio::io_context_base> context) const override;

        std::string get_ethererum_address() const override;

        std::string get_bootstrap_peers_file() const override;

        std::string get_bootstrap_peers_url() const override;

        std::string get_ethererum_io_api_token() const override;

        bool get_debug_logging() const override;

        bool get_log_to_stdout() const override;

        bzn::uuid_t get_uuid() const override;

        std::chrono::seconds get_ws_idle_timeout() const override;

        size_t get_audit_mem_size() const override;

        std::string get_state_dir() const override;

        std::string get_logfile_dir() const override;

        size_t get_max_storage() const override;

        bool get_mem_storage() const override;

        size_t get_logfile_rotation_size() const override ;

        size_t get_logfile_max_size() const override;

        bool pbft_enabled() const override;

        uint16_t get_http_port() const override;
        
        bool peer_validation_enabled() const override;

    private:
        size_t parse_size(const std::string& key) const;

        simple_options raw_opts;

    };

} // bzn
