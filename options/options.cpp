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
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <regex>
#include <iostream>
#include <fstream>
#include <swarm_version.hpp>

#ifndef __APPLE__
#include <optional>
#else
#include <experimental/optional>
#endif

namespace po = boost::program_options;
using namespace bzn;


namespace
{
    const std::string DEFAULT_CONFIG_FILE = "bluzelle.json";
    const std::string AUDIT_MEM_SIZE_KEY = "audit_mem_size";
    const std::string BOOTSTRAP_PEERS_FILE_KEY = "bootstrap_file";
    const std::string BOOTSTRAP_PEERS_URL_KEY = "bootstrap_url";
    const std::string DEBUG_LOGGING_KEY = "debug_logging";
    const std::string ETHERERUM_IO_API_TOKEN_KEY = "ethereum_io_api_token";
    const std::string ETHERERUM_KEY = "ethereum";
    const std::string HTTP_PORT_KEY = "http_port";
    const std::string LISTENER_ADDRESS_KEY = "listener_address";
    const std::string LISTENER_PORT_KEY = "listener_port";
    const std::string LOG_TO_STDOUT_KEY = "log_to_stdout";
    const std::string LOGFILE_DIR_KEY = "logfile_dir";
    const std::string LOGFILE_MAX_SIZE_KEY = "logfile_max_size";
    const std::string LOGFILE_ROTATION_SIZE_KEY = "logfile_rotation_size";
    const std::string MAX_STORAGE_KEY = "max_storage";
    const std::string MONITOR_ADDRESS_KEY = "monitor_address";
    const std::string MONITOR_PORT_KEY = "monitor_port";
    const std::string NODE_UUID = "uuid";
    const std::string PBFT_ENABLED_KEY = "use_pbft";
    const std::string STATE_DIR_KEY = "state_dir";
    const std::string WS_IDLE_TIMEOUT_KEY = "ws_idle_timeout";

    // this is 10k error strings in a vector, which is pessimistically 10MB, which is small enough that no one should mind
    const size_t DEFAULT_AUDIT_MEM_SIZE = 10000;

    const std::string DEFAULT_STATE_DIR = "./.state/";

    // Default logfile director
    const std::string DEFAULT_LOGFILE_DIR = "logs/";

    // The default maximum allowed storage for a node is 2G
    const size_t DEFAULT_MAX_STORAGE_SIZE = 2147483648;

    // Default log size before rotation
    const size_t DEFAULT_LOGFILE_ROATION_SIZE = 65536;

    // Default max log files before deletion
    const size_t DEFAULT_LOGFILE_MAX_SIZE = 524288;

    // Default http server port
    const uint16_t DEFAULT_HTTP_PORT = 8080;

    // https://stackoverflow.com/questions/8899069
    bool
    is_hex_notation(std::string const& s)
    {
        return s.compare(0, 2, "0x") == 0
               && s.size() > 2
               && s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
    }
}


bool
options::parse_command_line(int argc, const char* argv[])
{
    // no args then load default options...
    if (argc == 1)
    {
        this->load(DEFAULT_CONFIG_FILE);

        return this->validate();
    }

    // validate passed args...
    bool ret = this->parse(argc, argv);

    if (ret)
    {
        return this->validate();
    }
    return false;
}


boost::asio::ip::tcp::endpoint
options::get_listener() const
{
    try
    {
        auto ep = boost::asio::ip::tcp::endpoint{boost::asio::ip::address::from_string(this->config_data[LISTENER_ADDRESS_KEY].asString()),
                   uint16_t(this->config_data[LISTENER_PORT_KEY].asUInt())};

        return ep;
    }
    catch (std::exception& ex)
    {
        throw std::runtime_error(std::string("\nCould not create listener: ") + ex.what());
    }
}


bzn::optional<boost::asio::ip::udp::endpoint>
options::get_monitor_endpoint(std::shared_ptr<bzn::asio::io_context_base> context) const
{
    if (this->config_data.isMember(MONITOR_PORT_KEY) && this->config_data.isMember(MONITOR_ADDRESS_KEY))
    {
        try
        {
            boost::asio::ip::udp::resolver resolver(context->get_io_context());
            auto eps = resolver.resolve(boost::asio::ip::udp::v4(),
                                        this->config_data[MONITOR_ADDRESS_KEY].asString(),
                                        std::to_string(this->config_data[MONITOR_PORT_KEY].asUInt()));

            return bzn::optional<boost::asio::ip::udp::endpoint>{*eps.begin()};
        }
        catch (std::exception& ex)
        {
            throw std::runtime_error(std::string("\nCould not create monitor endpoint: ") + ex.what());
        }

    }
    else
    {
        return bzn::optional<boost::asio::ip::udp::endpoint>{};
    }
}

bool
options::pbft_enabled() const
{
    return this->config_data.isMember(PBFT_ENABLED_KEY) && this->config_data[PBFT_ENABLED_KEY].asBool();
}


std::string
options::get_ethererum_address() const
{
    return this->config_data[ETHERERUM_KEY].asString();
}


std::string
options::get_ethererum_io_api_token() const
{
    return this->config_data[ETHERERUM_IO_API_TOKEN_KEY].asString();
}


std::string
options::get_bootstrap_peers_file() const
{
    return this->config_data[BOOTSTRAP_PEERS_FILE_KEY].asString();
}


std::string
options::get_bootstrap_peers_url() const
{
    return this->config_data[BOOTSTRAP_PEERS_URL_KEY].asString();
}

bzn::uuid_t
options::get_uuid() const
{
    return this->config_data[NODE_UUID].asString();
}


void
options::load(const std::string& config_file)
{
    try
    {
        std::ifstream ifile(config_file);
        ifile.exceptions(std::ios::failbit);

        Json::Reader reader;
        if (!reader.parse(ifile, this->config_data))
        {
            throw std::runtime_error("Failed to parse: " + config_file + " : " + reader.getFormattedErrorMessages());
        }
    }
    catch (std::exception& /*e*/)
    {
        throw std::runtime_error("Failed to load: " + config_file + " : " + strerror(errno));
    }
}


bool
options::validate()
{
    // validate listener...
    if (!this->config_data.isMember(LISTENER_ADDRESS_KEY))
    {
        std::cerr << "Missing listener address entry!" << '\n';
        return false;
    }

    // validate port...
    if (!this->config_data.isMember(LISTENER_PORT_KEY))
    {
        std::cerr << "Missing listener port entry!" << '\n';
        return false;
    }
    else
    {
        auto port = this->config_data[LISTENER_PORT_KEY].asUInt();

        // todo: why do care what port is being used?
        if (!(port >= 49152 && port <= 65535))
        {
            std::cerr << "Invalid listener port entry: " << std::to_string(port) << '\n';
            return false;
        }
    }

    // validate Ethereum address...
    if (!this->config_data.isMember(ETHERERUM_KEY))
    {
        std::cerr << "Missing Ethereum address entry!" << '\n';
        return false;
    }
    else
    {
        if (!is_hex_notation(this->config_data[ETHERERUM_KEY].asString()))
        {
            std::cerr << "Invalid Ethereum address: " << this->config_data[ETHERERUM_KEY].asString() << '\n';
            return false;
        }
    }

    if (!this->config_data.isMember(ETHERERUM_IO_API_TOKEN_KEY))
    {
        std::cerr << "Missing Ethereum IO API token entry!" << '\n';
        return false;
    }

    if(this->config_data.isMember(MONITOR_PORT_KEY) && this->config_data.isMember(MONITOR_ADDRESS_KEY))
    {
        LOG(info) << "No monitor address provided; will not send monitor packets";
    }

    return true;
}


bool
options::get_debug_logging() const
{
    if (this->config_data.isMember(DEBUG_LOGGING_KEY))
    {
        return this->config_data[DEBUG_LOGGING_KEY].asBool();
    }

    return false;
}


bool
options::get_log_to_stdout() const
{
    if (this->config_data.isMember(LOG_TO_STDOUT_KEY))
    {
        return this->config_data[LOG_TO_STDOUT_KEY].asBool();
    }

    return false;
}


std::chrono::seconds
options::get_ws_idle_timeout() const
{
    return std::chrono::seconds(this->config_data[WS_IDLE_TIMEOUT_KEY].asUInt64());
}


size_t
options::get_audit_mem_size() const
{
    if (this->config_data.isMember(AUDIT_MEM_SIZE_KEY))
    {
        return this->config_data[AUDIT_MEM_SIZE_KEY].asUInt();
    }

    return DEFAULT_AUDIT_MEM_SIZE;
}


std::string
options::get_state_dir() const
{
    if (this->config_data.isMember(STATE_DIR_KEY))
    {
        return this->config_data[STATE_DIR_KEY].asString();
    }

    return DEFAULT_STATE_DIR;
}


std::string
options::get_logfile_dir() const
{
    if (this->config_data.isMember(LOGFILE_DIR_KEY))
    {
        return this->config_data[LOGFILE_DIR_KEY].asString();
    }

    return DEFAULT_LOGFILE_DIR;
}


bool
options::parse(int argc, const char* argv[])
{
    po::options_description desc("Options");

    // variables the map will fill...
    std::string config_file;

    desc.add_options()
        ("help,h",    "Shows this information")
        ("version,v", "Show the application's version")
        ("config,c",  po::value<std::string>(&config_file), "Path to configuration file");

    po::variables_map vm;

    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("version"))
        {
            std::cout << "Bluzelle" << ": v" << SWARM_VERSION << std::endl;
            return false;
        }

        if (vm.count("help") || vm.count("config") == 0)
        {
            std::cout << "Usage:" << '\n'
                      << "  " << "bluzelle" << " [OPTION]" << '\n'
                      << '\n' << desc << '\n';
            return false;
        }

        po::notify(vm);

        this->load(config_file);
        return true;
    }
    catch (po::error& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return false;
    }
    catch (std::exception& e)
    {
        std::cerr << "Unhandled Exception: " << e.what() << ", application will now exit" << std::endl;
        return false;
    }

    return true;
}


size_t
options::get_max_storage() const
{
    size_t value = DEFAULT_MAX_STORAGE_SIZE;

    if (auto conf_value = this->parse_size(MAX_STORAGE_KEY))
    {
        // todo: use operator* to workaround clang bug...
        value = *conf_value;
    }
    return value;
}


size_t
options::get_logfile_rotation_size() const
{
    size_t value = DEFAULT_LOGFILE_ROATION_SIZE;

    if (auto conf_value = this->parse_size(LOGFILE_ROTATION_SIZE_KEY))
    {
        // todo: use operator* to workaround clang bug...
        value = *conf_value;
    }
    return value;
}


size_t
options::get_logfile_max_size() const
{
    size_t value = DEFAULT_LOGFILE_MAX_SIZE;

    if (auto conf_value = this->parse_size(LOGFILE_MAX_SIZE_KEY))
    {
        // todo: use operator* to workaround clang bug...
        value = *conf_value;
    }
    return value;
}


#ifndef __APPLE__
std::optional<size_t>
#else
std::experimental::optional<size_t>
#endif
options::parse_size(const std::string& key) const
{
    if (this->config_data.isMember(key))
    {
        const std::string max_value{this->config_data[key].asString()};

        const std::regex expr{"(\\d+)([K,M,G,T]?[B]?)"};

        std::smatch base_match;
        if (std::regex_match(max_value, base_match, expr))
        {
            std::string suffix = base_match[2];

            return boost::lexical_cast<size_t>(base_match[1])
                   * utils::BYTE_SUFFIXES.at(suffix.empty() ? 'B' : suffix[0]);
        }

        throw std::runtime_error(std::string("\nUnable to parse \"" + key + "\" value from options: " + max_value));
    }

    return {}; /*std::nullopt*/
}


uint16_t
options::get_http_port() const
{
    if (this->config_data.isMember(HTTP_PORT_KEY))
    {
        return this->config_data[HTTP_PORT_KEY].asInt();
    }

    return DEFAULT_HTTP_PORT;
}