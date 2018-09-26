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


// gmock_gen.py generated...

namespace bzn {

class Mocknode_base : public node_base {
 public:
  MOCK_METHOD2(register_for_message,
      bool(const std::string& msg_type, bzn::message_handler message_handler));
  MOCK_METHOD2(register_for_message,
      bool(const bzn_msg_type msg_type, bzn::protobuf_handler message_handler));
  MOCK_METHOD0(start,
      void());
  MOCK_METHOD2(send_message,
      void(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<bzn::message> msg));
  MOCK_METHOD2(send_message_str,
      void(const boost::asio::ip::tcp::endpoint& ep, std::shared_ptr<std::string> msg));
};

}  // namespace bzn
