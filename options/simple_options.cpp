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

#include <options/simple_options.hpp>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <swarm_version.hpp>
#include <boost/algorithm/string.hpp>

using namespace bzn;
using namespace bzn::option_names;
namespace po = boost::program_options;

namespace
{
    const std::string DEFAULT_CONFIG_FILE = "bluzelle.json";

    // https://stackoverflow.com/questions/8899069
    bool
    is_hex_notation(std::string const& s)
    {
        return s.compare(0, 2, "0x") == 0
               && s.size() > 2
               && s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
    }
}

simple_options::simple_options()
        : options_root("Core configuration")
{
    this->build_options();
}

bool
simple_options::parse(int argc, const char* argv[])
{
    return this->handle_command_line_options(argc, argv) && this->handle_config_file_options() && this->validate_options();
}

void
simple_options::build_options()
{
    this->options_root.add_options()
                (BOOTSTRAP_PEERS_FILE.c_str(),
                        po::value<std::string>(),
                        "file path for bootstrap peers list")
                (BOOTSTRAP_PEERS_URL.c_str(),
                        po::value<std::string>(),
                        "url for bootstrap peers list")
                (ETHERERUM_IO_API_TOKEN.c_str(),
                        po::value<std::string>()->required(),
                        "ethereum io api token")
                (ETHERERUM.c_str(),
                        po::value<std::string>()->required(),
                        "ethereum address")
                (HTTP_PORT.c_str(),
                        po::value<uint16_t>()->default_value(8080),
                        "port for http server")
                (LISTENER_ADDRESS.c_str(),
                        po::value<std::string>()->required(),
                        "listener address for consensus node")
                (LISTENER_PORT.c_str(),
                        po::value<uint16_t>()->required(),
                        "port for consensus node")
                (MAX_STORAGE.c_str(),
                         po::value<std::string>()->default_value("2G"),
                        "maximum db storage on this node (bytes)")
                (MEM_STORAGE.c_str(),
                         po::value<bool>()->default_value(true),
                         "enable in memory storage for debugging")
                (NODE_UUID.c_str(),
                        po::value<std::string>(),
                        "uuid of this node")
                (STATE_DIR.c_str(),
                        po::value<std::string>()->default_value("./.state/"),
                        "location for state files")
                (WS_IDLE_TIMEOUT.c_str(),
                        po::value<uint64_t>()->default_value(300000),
                        "websocket idle timeout (ms)");

    po::options_description logging("Logging");
    logging.add_options()
                (LOG_TO_STDOUT.c_str(),
                        po::value<bool>()->default_value(false),
                        "log to stdout")
                (LOGFILE_DIR.c_str(),
                        po::value<std::string>()->default_value("logs/"),
                        "directory for log files")
                (LOGFILE_MAX_SIZE.c_str(),
                        po::value<std::string>()->default_value("512K"),
                        "maximum size for log files")
                (LOGFILE_ROTATION_SIZE.c_str(),
                        po::value<std::string>()->default_value("64K"),
                        "size at which to rotate log files")
                (DEBUG_LOGGING.c_str(),
                        po::value<bool>()->default_value(false),
                        "enable debug logging");
    this->options_root.add(logging);

    po::options_description audit("Audit");
    audit.add_options()
                (AUDIT_ENABLED.c_str(),
                        po::value<bool>()->default_value(true),
                        "enable audit module")
                (AUDIT_MEM_SIZE.c_str(),
                        po::value<size_t>()->default_value(10000),
                        "max size of audit datastructures")
                (MONITOR_ADDRESS.c_str(),
                        po::value<std::string>(),
                        "address of stats.d listener for audit module")
                (MONITOR_PORT.c_str(),
                        po::value<uint16_t>(),
                        "port of stats.d listener for audit module")
                (MONITOR_COLLATE.c_str(),
                        po::value<bool>()->default_value(true),
                        "send monitor packets at an interval to reduce packet count (and log spam)")
                (MONITOR_COLLATE_INTERVAL_SECONDS.c_str(),
                        po::value<uint64_t>()->default_value(5),
                        "interval at which to send monitor packets (if collation is enabled)")
                (MONITOR_MAX_TIMERS.c_str(),
                        po::value<uint64_t>(),
                        "maximum number of outstanding monitor timers");
    this->options_root.add(audit);

    po::options_description experimental("Experimental");
    experimental.add_options()
                (PEER_VALIDATION_ENABLED.c_str(),
                        po::value<bool>()->default_value(false),
                        "require signed key for new peers to join swarm")
                (SIGNED_KEY.c_str(),
                        po::value<std::string>(),
                        "signed key for node's uuid");
    this->options_root.add(experimental);

    po::options_description crypto("Cryptography");
    crypto.add_options()
                (NODE_PUBKEY_FILE.c_str(),
                        po::value<std::string>()->default_value(".state/public-key.pem"),
                        "public key of this node")
                (NODE_PRIVATEKEY_FILE.c_str(),
                        po::value<std::string>()->default_value(".state/private-key.pem"),
                        "private key of this node")
                (CRYPTO_ENABLED_INCOMING.c_str(),
                        po::value<bool>()->default_value(true),
                        "check signatures on incoming messages")
                (CRYPTO_ENABLED_OUTGOING.c_str(),
                         po::value<bool>()->default_value(true),
                        "attach signatures on outgoing messages")
                (CRYPTO_SELF_VERIFY.c_str(),
                         po::value<bool>()->default_value(true),
                        "verify own signatures as a sanity check");


    this->options_root.add(crypto);

    po::options_description chaos("Chaos testing");
    chaos.add_options()
                 (CHAOS_ENABLED.c_str(),
                         po::value<bool>()->default_value(false),
                         "enable chaos testing module")

                 /*
                  * With the default parameters specified here,
                  * 10% of nodes will fail within a couple minutes
                  * 20% more will fail within the first hour
                  * 40% will last 1-12 hours
                  * 20% will last 12-48 hours
                  * 10% will last 48+ hours
                  */
                 (CHAOS_NODE_FAILURE_SHAPE.c_str(),
                         po::value<double>()->default_value(0.5),
                         "shape parameter of Weibull distribution for node lifetime")
                 (CHAOS_NODE_FAILURE_SCALE.c_str(),
                         po::value<double>()->default_value(10),
                         "scale parameter of Weibull distribution for node lifetime (expressed in hours)")


                 /*
                  * These parameters are chosen arbitrarily, but a cursory search suggests that
                  * an exponential distribution is indeed reasonable for internet packet delay
                  */
                 (CHAOS_MESSAGE_DELAY_CHANCE.c_str(),
                         po::value<double>()->default_value(0.1),
                         "chance by which outgoing messages are delayed (independently at random)")
                 (CHAOS_MESSAGE_DELAY_TIME.c_str(),
                         po::value<uint>()->default_value(2500),
                         "how long to wait before attempting to resend a delayed message (at which point it can be delayed again, resulting in an exponential distribution on the actual delay)")


                 (CHAOS_MESSAGE_DROP_CHANCE.c_str(),
                         po::value<double>()->default_value(0.025),
                         "chance that outgoing messages are dropped entirely (independently at random)");

    this->options_root.add(chaos);
}

bool
simple_options::validate_options()
{
    bool errors = false;

    // Boost will enforce required-ness and types of options, we only need to validate more complex constraints

    auto port = this->get<uint16_t>(LISTENER_PORT);
    if (port <= 1024)
    {
        std::cerr << "Invalid listener port " << std::to_string(port);
        errors = true;
    }

    auto eth_address = this->get<std::string>(ETHERERUM);
    if (!is_hex_notation(eth_address))
    {
        std::cerr << "Invalid Ethereum address " << eth_address;
        errors = true;
    }

    if (! (this->has(BOOTSTRAP_PEERS_FILE) || this->has(BOOTSTRAP_PEERS_URL)))
    {
        std::cerr << "Bootstrap peers source not specified";
        errors = true;
    }

    if (!this->vm[NODE_PUBKEY_FILE].defaulted() && this->has(NODE_UUID))
    {
        std::cerr << "You cannot specify both a uuid and a public key; public keys act as uuids";
        errors = true;
    }

    return !errors;
}

bool
simple_options::handle_config_file_options()
{
    Json::Value json;

    std::ifstream ifile;
    ifile.exceptions(std::ios::failbit);

    try
    {
        ifile.open(config_file);
    }
    catch (const std::exception& /*ex*/)
    {
        throw std::runtime_error("Failed to load: " + config_file + " : " + strerror(errno));
    }

    Json::Reader reader;
    if (!reader.parse(ifile, json))
    {
        throw std::runtime_error("Failed to parse: " + config_file + " : " + reader.getFormattedErrorMessages());
    }

    if (!json.isObject())
    {
        throw std::runtime_error("Config file should be an object");
    }

    po::parsed_options parsed(&(this->options_root));

    for (const auto& name : json.getMemberNames())
    {
        const auto& json_val = json[name];

        if (! this->options_root.find_nothrow(name.c_str(), false))
        {
            std::cerr << "Warning: ignoring unknown config file option '" << name << "'\n";
            continue;
        }

        boost::program_options::basic_option<char> opt;
        opt.string_key = name;

        if (json_val.isString())
        {
            opt.value.push_back(json_val.asString());
        }
        else
        {
            // If it's not a string, then it should be a bool or a number, so we can let boost parse it
            auto option_value = json_val.toStyledString();
            boost::trim(option_value); // jsoncpp sometimes includes a trailing newline here

            opt.value.push_back(option_value);
        }

        parsed.options.push_back(opt);
    }

    boost::program_options::store(parsed, vm);
    boost::program_options::notify(vm);

    return true;
}

bool
simple_options::handle_command_line_options(int argc, const char* argv[])
{
    boost::program_options::options_description desc("Command line options");

    desc.add_options()
                ("help,h",    "Shows this information")
                ("version,v", "Show the application's version")
                ("config,c",  po::value<std::string>(&this->config_file)->default_value(DEFAULT_CONFIG_FILE), "Path to configuration file");

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
                      << '\n' << desc << '\n'

                      << "Config file options (specified as keys in JSON object, not command line arguments)" << '\n'
                      << '\n' << this->options_root << '\n';

            return false;
        }

        po::notify(vm);

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
}

bool
simple_options::has(const std::string& option_name) const
{
    return this->vm.count(option_name) > 0;
}

void
simple_options::set(const std::string& option_name, const std::string& option_value)
{
    po::basic_option<char> opt;
    opt.string_key = option_name;
    opt.value.push_back(option_value);

    boost::program_options::parsed_options parsed(&(this->options_root));
    parsed.options.push_back(opt);

    this->vm.erase(option_name);
    po::store(parsed, this->vm);
}
