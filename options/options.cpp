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
#include <iostream>
#include <fstream>

namespace po = boost::program_options;
using namespace bzn;

namespace
{
    const std::string DEFAULT_CONFIG_FILE        = "bluzelle.json";
    const std::string LISTENER_ADDRESS_KEY       = "listener_address";
    const std::string LISTENER_PORT_KEY          = "listener_port";
    const std::string ETHERERUM_KEY              = "ethereum";
    const std::string BOOTSTRAP_PEERS_FILE_KEY   = "bootstrap_file";
    const std::string BOOTSTRAP_PEERS_URL_KEY    = "bootstrap_url";
    const std::string DEBUG_LOGGING_KEY          = "debug_logging";
    const std::string LOG_TO_STDOUT_KEY          = "log_to_stdout";

    // https://stackoverflow.com/questions/8899069
    bool is_hex_notation(std::string const& s)
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
    catch(std::exception& ex)
    {
        throw std::runtime_error(std::string("\nCould not create listener: ") + ex.what());
    }
}


std::string
options::get_ethererum_address() const
{
    return this->config_data[ETHERERUM_KEY].asString();
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
    return this->config_data["uuid"].asString();
}


void
options::load(const std::string& config_file)
{
    LOG(debug) << "Loading: " << config_file;

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
    catch(std::exception& /*e*/)
    {
        throw std::runtime_error("Failed to load: " + config_file + " : " + strerror(errno));
    }
}


bool
options::validate()
{
    LOG(info) << '\n' << this->config_data.toStyledString();

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


bool
options::parse(int argc, const char* argv[])
{
    po::options_description desc("Options");

    // variables the map will fill...
    std::string config_file;
    std::string ethereum;
    std::string listener_address;
    std::string bootstrap_file;
    std::string bootstrap_url;
    uint32_t listener_port{};

    desc.add_options()
        ("help,h",             "Shows this information")
        ("version,v",          "Show the application's version")
        ("address,a",          po::value<std::string>(&ethereum),        " Ethereum account address")
        ("listener_address,l", po::value<std::string>(&listener_address), "Listener IP address")
        ("listener_port,p",    po::value<uint32_t>(&listener_port),       "Listener port")
        ("bootstrap_file,b",   po::value<std::string>(&bootstrap_file),   "Path to initial peers file")
        ("bootstrap_url",      po::value<std::string>(&bootstrap_url),    "URL to fetch initial peers")
        ("config,c",           po::value<std::string>(&config_file),      "Path to configuration file");

    po::variables_map vm;

    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << "Usage:" << '\n'
                      << "  " << "bluzelle" << " [OPTION]" << '\n'
                      << '\n' << desc << '\n';
            return false;
        }

        if (vm.count("version"))
        {
            std::cout << "Bluzelle" << ": v" << BLUZELLE_VERSION << std::endl;
            return false;
        }

        po::notify(vm);

        // if config is specified then ignore everything else...
        if (vm.count("config"))
        {
            this->load(config_file);
            return true;
        }

        if (!listener_address.empty())
        {
            // don't want to create the entry unless there is data to set...
            this->config_data[LISTENER_ADDRESS_KEY]= listener_address;
        }
        this->config_data[LISTENER_PORT_KEY] = listener_port;
        this->config_data[ETHERERUM_KEY] = ethereum;
        this->config_data[BOOTSTRAP_PEERS_FILE_KEY] = bootstrap_file;
        this->config_data[BOOTSTRAP_PEERS_URL_KEY] = bootstrap_url;
    }
    catch(po::error& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return false;
    }
    catch(std::exception& e)
    {
        std::cerr << "Unhandled Exception: " << e.what() << ", application will now exit" << std::endl;
        return false;
    }

    return true;
}
