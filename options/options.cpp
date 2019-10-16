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

#include <options/options.hpp>
#include <boost/lexical_cast.hpp>
#include <regex>
#include <cstdint>
#include <utils/crypto.hpp>
#include <chrono>

using namespace bzn;
using namespace bzn::option_names;

bool
options::parse_command_line(int argc, const char* argv[])
{
    return this->raw_opts.parse(argc, argv);
}

const simple_options&
options::get_simple_options() const
{
    return this->raw_opts;
}

simple_options&
options::get_mutable_simple_options()
{
    return this->raw_opts;
}

boost::asio::ip::tcp::endpoint
options::get_listener() const
{
    try
    {
        auto ep = boost::asio::ip::tcp::endpoint{
            boost::asio::ip::address::from_string(this->raw_opts.get<std::string>(LISTENER_ADDRESS)),
            this->raw_opts.get<uint16_t>(LISTENER_PORT)};

        return ep;
    }
    catch (std::exception& ex)
    {
        throw std::runtime_error(std::string("\nCould not create listener: ") + ex.what());
    }
}


std::optional<boost::asio::ip::udp::endpoint>
options::get_monitor_endpoint(std::shared_ptr<bzn::asio::io_context_base> context) const
{
    if (this->raw_opts.has(MONITOR_PORT) && this->raw_opts.has(MONITOR_ADDRESS))
    {
        try
        {
            boost::asio::ip::udp::resolver resolver(context->get_io_context());
            auto eps = resolver.resolve(boost::asio::ip::udp::v4(),
                                        this->raw_opts.get<std::string>(MONITOR_ADDRESS),
                                        std::to_string(this->raw_opts.get<uint16_t>(MONITOR_PORT)));

            return std::optional<boost::asio::ip::udp::endpoint>{*eps.begin()};
        }
        catch (std::exception& ex)
        {
            throw std::runtime_error(std::string("\nCould not create monitor endpoint: ") + ex.what());
        }

    }
    else
    {
        return std::optional<boost::asio::ip::udp::endpoint>{};
    }
}


std::string
options::get_bootstrap_peers_file() const
{
    //TODO: Remove this
    return this->raw_opts.get<std::string>(BOOTSTRAP_PEERS_FILE);
}


std::string
options::get_bootstrap_peers_url() const
{
    //TODO: Remove this
    return this->raw_opts.get<std::string>(BOOTSTRAP_PEERS_URL);
}

bzn::uuid_t
options::get_uuid() const
{
    if (this->raw_opts.has(NODE_UUID))
    {
        return this->raw_opts.get<std::string>(NODE_UUID);
    }

    std::string pubkey_raw = bzn::utils::crypto::read_pem_file(this->raw_opts.get<std::string>(NODE_PUBKEY_FILE), "PUBLIC KEY");
    return boost::beast::detail::base64_encode(pubkey_raw);
}

bzn::swarm_id_t
options::get_swarm_id() const
{
    return this->raw_opts.get<std::string>(SWARM_ID);
}

bool
options::get_debug_logging() const
{
    //TODO: Remove this
    return this->raw_opts.get<bool>(DEBUG_LOGGING);
}


bool
options::get_log_to_stdout() const
{
    //TODO: Remove this
    return this->raw_opts.get<bool>(LOG_TO_STDOUT);
}


std::chrono::milliseconds
options::get_ws_idle_timeout() const
{
    //TODO: Remove this?
    return std::chrono::milliseconds(raw_opts.get<uint64_t>(WS_IDLE_TIMEOUT));
}


size_t
options::get_audit_mem_size() const
{
    //TODO: Remove this
    return this->raw_opts.get<size_t>(AUDIT_MEM_SIZE);
}


std::string
options::get_state_dir() const
{
    //TODO: Remove this
    return this->raw_opts.get<std::string>(STATE_DIR);
}


std::string
options::get_logfile_dir() const
{
    //TODO: Remove this
    return this->raw_opts.get<std::string>(LOGFILE_DIR);
}


size_t
options::get_max_swarm_storage() const
{
    return this->parse_size(this->raw_opts.get<std::string>(MAX_SWARM_STORAGE));
}


bool
options::get_mem_storage() const
{
    return this->raw_opts.get<bool>(MEM_STORAGE);
}


size_t
options::get_logfile_rotation_size() const
{
    return this->parse_size(this->raw_opts.get<std::string>(LOGFILE_ROTATION_SIZE));
}


size_t
options::get_logfile_max_size() const
{
    return this->parse_size(this->raw_opts.get<std::string>(LOGFILE_MAX_SIZE));
}


size_t
options::parse_size(const std::string& max_value) const
{
    const std::regex expr{"(\\d+)([K,M,G,T]?[B]?)"};

    std::smatch base_match;
    if (std::regex_match(max_value, base_match, expr))
    {
        std::string suffix = base_match[2];

        return boost::lexical_cast<size_t>(base_match[1])
               * utils::BYTE_SUFFIXES.at(suffix.empty() ? 'B' : suffix[0]);
    }

    throw std::runtime_error(std::string("\nUnable to parse size value from options: " + max_value));
}


bool 
options::peer_validation_enabled() const
{
    //TODO: Remove this
    return this->raw_opts.get<bool>(PEER_VALIDATION_ENABLED);
}


std::string
options::get_signed_key() const
{
    return this->raw_opts.get<std::string>(SIGNED_KEY);
}


std::string
options::get_owner_public_key() const
{
    return this->raw_opts.has(OWNER_PUBLIC_KEY) ? this->raw_opts.get<std::string>(OWNER_PUBLIC_KEY) : "";
}


std::string
options::get_swarm_info_esr_address() const
{
    return this->raw_opts.get<std::string>(SWARM_INFO_ESR_ADDRESS);
}


std::string
options::get_swarm_info_esr_url() const
{
    return this->raw_opts.get<std::string>(SWARM_INFO_ESR_URL);
}


std::string
options::get_stack() const
{
    return this->raw_opts.get<std::string>(STACK);
}

size_t
options::get_admission_window() const
{
    return this->raw_opts.get<size_t>(ADMISSION_WINDOW);
}

bool
options::get_peer_message_signing() const
{
    return this->raw_opts.get<size_t>(PEER_MESSAGE_SIGNING);
}