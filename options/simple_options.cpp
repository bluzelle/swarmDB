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

#include <include/bluzelle.hpp>
#include <options/simple_options.hpp>
#include <json/json.h>
#include <swarm_git_commit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/predef.h>
#include <iostream>
#include <fstream>

using namespace bzn;
using namespace bzn::option_names;
namespace po = boost::program_options;

namespace
{
    const std::string DEFAULT_CONFIG_FILE = "bluzelle.json";
}

simple_options::simple_options()
        : options_root("Core configuration")
        , cmd_opts(&(this->options_root))
        , config_file_opts(&(this->options_root))
{
    this->build_options();
}

bool
simple_options::parse(int argc, const char* argv[])
{
    return this->handle_command_line_options(argc, argv) && this->handle_config_file_options() && this->combine_options() && this->validate_options();
}

void
simple_options::build_options()
{
    po::options_description cmd_only("Command line only");
    cmd_only.add_options()
                    ("help,h", "Shows this information")
                    ("version,v", "Shows the application's version")
                    ("config,c", po::value<std::string>()->default_value(DEFAULT_CONFIG_FILE), "Path to configuration file");
    this->options_root.add(cmd_only);

    this->options_root.add_options()
                (BOOTSTRAP_PEERS_FILE.c_str(),
                        po::value<std::string>(),
                        "file path for bootstrap peers list")
                (BOOTSTRAP_PEERS_URL.c_str(),
                        po::value<std::string>(),
                        "url for bootstrap peers list")
                (PEERS_REFRESH_INTERVAL_SECONDS.c_str(),
                        po::value<uint64_t>()->default_value(600),
                        "interval at which to check for updates to the peers list")
                (LISTENER_ADDRESS.c_str(),
                        po::value<std::string>()->required(),
                        "listener address for consensus node")
                (LISTENER_PORT.c_str(),
                        po::value<uint16_t>()->required(),
                        "port for consensus node")
                (MAX_SWARM_STORAGE.c_str(),
                         po::value<std::string>()->default_value("0"),
                        "maximum db storage (bytes) in the swarm (default value of zero indicates no limit)")
                (MEM_STORAGE.c_str(),
                         po::value<bool>()->default_value(true),
                         "enable in memory storage for debugging")
                (NODE_UUID.c_str(),
                        po::value<std::string>(),
                        "uuid of this node")
                (SWARM_ID.c_str(),
                        po::value<std::string>()->required(),
                        "swarm id of this node")
                (STATE_DIR.c_str(),
                        po::value<std::string>()->default_value("./.state/"),
                        "location for state files")
                (OVERRIDE_NUM_THREADS.c_str(),
                        po::value<size_t>(),
                        "number of worker threads to run (default is automatic based on hardware")
                (WS_IDLE_TIMEOUT.c_str(),
                        po::value<uint64_t>()->default_value(300000),
                        "websocket idle timeout (ms)")
                (FD_OPER_TIMEOUT.c_str(),
                        po::value<uint64_t>()->default_value(30000),
                        "failure-detector operation timeout (ms)")
                (FD_FAIL_TIMEOUT.c_str(),
                        po::value<uint64_t>()->default_value(300000),
                        "failure-detector second failure timeout (ms)")
                (SWARM_INFO_ESR_ADDRESS.c_str(),
                        po::value<std::string>()->default_value(bzn::utils::DEFAULT_SWARM_INFO_ESR_ADDRESS),
                        "Address of ESR Swarm Info contract")
                (SWARM_INFO_ESR_URL.c_str(),
                        po::value<std::string>()->default_value(bzn::utils::ROPSTEN_URL),
                        "url of ESR Swarm Info contract server")
                (IGNORE_ESR.c_str(),
                        po::value<bool>()->default_value(false),
                        "do not use esr as a peer source")
                (STACK.c_str(),
                        po::value<std::string>()->required(),
                        "software stack used by swarm");

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
                        "signed key for node's uuid")
                (OWNER_PUBLIC_KEY.c_str(),
                        po::value<std::string>(),
                        "swarm owner's public key");

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

    po::options_description wss("Websocket Secure");
    wss.add_options()
                (WSS_ENABLED.c_str(),
                    po::value<bool>()->default_value(false),
                    "enable websocket secure")
                (WSS_SERVER_CERTIFICATE_FILE.c_str(),
                    po::value<std::string>()->default_value(".state/wss-cert.pem"),
                    "wss server certificate")
                (WSS_SERVER_PRIVATE_KEY_FILE.c_str(),
                    po::value<std::string>()->default_value(".state/wss-private-key.pem"),
                    "wss server private key")
                (WSS_SERVER_DH_PARAMS_FILE.c_str(),
                    po::value<std::string>(),
                    "Diffie–Hellman params");

    this->options_root.add(wss);

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
    this->vm.notify();
    bool errors = false;

    // Boost will enforce required-ness and types of options, we only need to validate more complex constraints

    auto port = this->get<uint16_t>(LISTENER_PORT);
    if (port <= 1024)
    {
        std::cerr << "Invalid listener port " << std::to_string(port);
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

    this->config_file_opts = po::parsed_options{&(this->options_root)};

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

        this->config_file_opts.options.push_back(opt);
    }

    return true;
}

bool
simple_options::handle_command_line_options(int argc, const char* argv[])
{
    try
    {
        this->cmd_opts = po::parse_command_line(argc, argv, this->options_root);
        po::variables_map early_vm;
        po::store(this->cmd_opts, early_vm);

        if (early_vm.count("version"))
        {
            std::cout << "swarmdb" << ": " << SWARM_GIT_COMMIT << std::endl;

            std::string compiler_name = "unknown";
            if (BOOST_COMP_CLANG)
            {
                compiler_name = "clang";
            }

            if (BOOST_COMP_GNUC){
                compiler_name = "gcc";
            }

            if (BOOST_COMP_LLVM){
                compiler_name = "llvm";
            }

#ifdef __OPTIMIZE__
            bool opt = true;
#else
            bool opt = false;
#endif

            std::cout << "compiled by " << compiler_name <<  __VERSION__ << "; optimize=" << opt << std::endl;

            return false;
        }

        if (early_vm.count("help") || early_vm.count("config") == 0)
        {
            std::cout << "Usage:" << '\n'
                      << "  " << "bluzelle" << " [OPTION]" << '\n'
                      << "Long form options may be specified on command line or in json object in config file" << '\n'
                      << '\n' << this->options_root << '\n';

            return false;
        }

        this->config_file = early_vm["config"].as<std::string>();
        return true;
    }
    catch (po::error& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        return false;
    }
    catch (std::exception& e)
    {
        std::cerr << "Unhandled Exception: " << e.what() << ", application will now exit" << std::endl;
        return false;
    }
}

bool
simple_options::combine_options()
{
    boost::program_options::parsed_options override_opts(&(this->options_root));
    for (const auto& pair : this->overrides)
    {
        po::basic_option<char> opt;
        opt.string_key = pair.first;
        opt.value.push_back(pair.second);
        override_opts.options.push_back(opt);
    }

    this->vm = boost::program_options::variables_map{};
    po::store(override_opts, this->vm);
    po::store(this->cmd_opts, this->vm);
    po::store(this->config_file_opts, this->vm);

    return true;
}

bool
simple_options::has(const std::string& option_name) const
{
    return this->vm.count(option_name) > 0;
}

void
simple_options::set(const std::string& option_name, const std::string& option_value)
{
    std::lock_guard<std::mutex> lock(this->lock);
    this->overrides[option_name] = option_value;
    this->combine_options();
}

