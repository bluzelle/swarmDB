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

#include <node/node_base.hpp>
#include <gmock/gmock.h>
#include <options/options_base.hpp>

namespace bzn {

class mock_options_base : public options_base {
 public:
  MOCK_CONST_METHOD0(get_simple_options,
      const simple_options&());
  MOCK_METHOD0(get_mutable_simple_options,
      simple_options&());
  MOCK_CONST_METHOD0(get_listener,
      boost::asio::ip::tcp::endpoint());
  MOCK_CONST_METHOD1(get_monitor_endpoint,
      std::optional<boost::asio::ip::udp::endpoint>(std::shared_ptr<bzn::asio::io_context_base> context));
  MOCK_CONST_METHOD0(get_ethererum_address,
      std::string());
  MOCK_CONST_METHOD0(get_ethererum_io_api_token,
      std::string());
  MOCK_CONST_METHOD0(get_bootstrap_peers_url,
      std::string());
  MOCK_CONST_METHOD0(get_bootstrap_peers_file,
      std::string());
  MOCK_CONST_METHOD0(get_debug_logging,
      bool());
  MOCK_CONST_METHOD0(get_log_to_stdout,
      bool());
  MOCK_CONST_METHOD0(get_uuid,
      bzn::uuid_t());
  MOCK_CONST_METHOD0(get_swarm_id,
        bzn::swarm_id_t());
  MOCK_CONST_METHOD0(get_ws_idle_timeout,
      std::chrono::milliseconds());
    MOCK_CONST_METHOD0(get_fd_oper_timeout,
        std::chrono::milliseconds());
    MOCK_CONST_METHOD0(get_fd_fail_timeout,
        std::chrono::milliseconds());
  MOCK_CONST_METHOD0(get_audit_mem_size,
      size_t());
  MOCK_CONST_METHOD0(get_state_dir,
      std::string());
  MOCK_CONST_METHOD0(get_logfile_dir,
      std::string());
  MOCK_CONST_METHOD0(get_max_swarm_storage,
        size_t());
  MOCK_CONST_METHOD0(get_mem_storage,
      bool());
  MOCK_CONST_METHOD0(get_logfile_rotation_size,
      size_t());
  MOCK_CONST_METHOD0(get_logfile_max_size,
      size_t());
  MOCK_CONST_METHOD0(pbft_enabled,
      bool());
  MOCK_CONST_METHOD0(peer_validation_enabled,
      bool());
  MOCK_CONST_METHOD0(get_signed_key,
      std::string());
  MOCK_CONST_METHOD0(get_owner_public_key,
      std::string());
  MOCK_CONST_METHOD0(get_swarm_info_esr_address,
      std::string());
  MOCK_CONST_METHOD0(get_swarm_info_esr_url,
      std::string());
  MOCK_CONST_METHOD0(get_stack,
      std::string());
  MOCK_CONST_METHOD0(get_wss_enabled,
      bool());
  MOCK_CONST_METHOD0(get_wss_server_certificate_file,
      std::string());
  MOCK_CONST_METHOD0(get_wss_server_private_key_file,
      std::string());
  MOCK_CONST_METHOD0(get_wss_server_dh_params_file,
      std::string());
};

}  // namespace bzn
