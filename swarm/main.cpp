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

#include <audit/audit.hpp>
#include <bootstrap/bootstrap_peers.hpp>
#include <chaos/chaos.hpp>
#include <crud/crud.hpp>
#include <crud/raft_crud.hpp>
#include <crud/subscription_manager.hpp>
#include <crypto/crypto.hpp>
#include <crypto/crypto_base.hpp>
#include <ethereum/ethereum.hpp>
#include <http/server.hpp>
#include <node/node.hpp>
#include <options/options.hpp>
#include <options/simple_options.hpp>
#include <pbft/pbft.hpp>
#include <pbft/database_pbft_service.hpp>
#include <pbft/pbft_failure_detector.hpp>
#include <raft/raft.hpp>
#include <status/status.hpp>
#include <storage/mem_storage.hpp>
#include <storage/rocksdb_storage.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/program_options.hpp>
#include <thread>


void
init_logging(const bzn::options& options)
{
    namespace keywords = boost::log::keywords;

    const auto format = boost::log::expressions::stream
            << boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "[%Y-%m-%d %H:%M:%S.%f UTC]")
            << " [" << boost::log::expressions::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID")
            << "] [" << std::setw(5) << std::left << boost::log::trivial::severity << "] " << boost::log::expressions::smessage;

    auto sink = boost::log::add_file_log
        (
            keywords::file_name = options.get_logfile_dir() + "/bluzelle-%5N.log",
            keywords::rotation_size = options.get_logfile_rotation_size(),
            keywords::open_mode = std::ios_base::app,
            keywords::auto_flush = true,
            keywords::format = format
        );

    boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::utc_clock());

    if (options.get_log_to_stdout())
    {
        boost::log::add_console_log(std::cout, boost::log::keywords::format = format);
    }

    boost::log::add_common_attributes();

    sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector
        (
            keywords::target = options.get_logfile_dir(),
            keywords::max_size = options.get_logfile_max_size()
        ));

    sink->locked_backend()->scan_for_files();

    boost::log::core::get()->add_sink(sink);

    if (options.get_debug_logging())
    {
        LOG(info) << "debug logging enabled";

        boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
    }
    else
    {
        LOG(info) << "debug logging disabled";

        boost::log::core::get()->set_filter(boost::log::trivial::severity > boost::log::trivial::debug);
    }
}


bool
init_peers(bzn::bootstrap_peers& peers, const std::string& peers_file, const std::string& peers_url)
{
    if (peers_file.empty() && peers_url.empty())
    {
        LOG(error) << "Bootstrap peers must be specified options (bootstrap_file or bootstrap_url)";
        return false;
    }

    if (!peers_file.empty())
    {
        peers.fetch_peers_from_file(peers_file);
    }

    if (!peers_url.empty())
    {
        peers.fetch_peers_from_url(peers_url);
    }

    if (peers.get_peers().empty())
    {
        LOG(error) << "Failed to find any bootstrap peers";
        return false;
    }
    return true;
}


size_t
get_state_file_size(const bzn::options& options)
{
    boost::filesystem::path state_file_path{options.get_state_dir() + "/" + options.get_uuid() + ".dat"};
    if (boost::filesystem::exists(state_file_path))
    {
        return size_t(boost::filesystem::file_size(state_file_path));
    }
    return 0;
}


void
print_banner(const bzn::options& options, double eth_balance)
{
    std::stringstream ss;

    ss << '\n';
    ss << "  Running node with ID: " << options.get_uuid() << "\n"
       << "   Ethereum Address ID: " << options.get_ethererum_address()  << "\n"
       << "      Local IP Address: " << options.get_listener().address().to_string() << "\n"
       << "               On port: " << options.get_listener().port() << "\n"
       << "             HTTP port: " << options.get_http_port()  << "\n"
       << "         Token Balance: " << eth_balance << " ETH" << "\n"
       << "       Maximum Storage: " << options.get_max_storage() << " Bytes" << "\n"
       << "          Used Storage: " << get_state_file_size(options) << " Bytes" << "\n"
       << '\n';

    std::cout << ss.str();

    if (!options.get_log_to_stdout())
    {
        LOG(info) << ss.str();
    }
}


void
start_worker_threads_and_wait(std::shared_ptr<bzn::asio::io_context_base> io_context)
{
    std::vector<std::thread> workers;

    for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
    {
        workers.emplace_back(std::thread([io_context]
        {
            io_context->run();
        }));
    }

    // wait for shutdown...
    for (auto& t : workers)
    {
        t.join();
    }
}


int
main(int argc, const char* argv[])
{
    try
    {
        auto options = std::make_shared<bzn::options>();
        if (!options->parse_command_line(argc, argv))
        {
            return 1;
        }

        init_logging(*options);

        // todo: right now we just want to check that an account "has" a balance...
        double eth_balance = bzn::ethereum().get_ether_balance(options->get_ethererum_address(), options->get_ethererum_io_api_token());

        if (eth_balance == 0)
        {
            LOG(error) << "No ETH balance found";
            return 0;
        }

        bzn::bootstrap_peers peers(options->peer_validation_enabled());
        if (!init_peers(peers, options->get_bootstrap_peers_file(), options->get_bootstrap_peers_url()))
            throw std::runtime_error("Bootstrap peers initialization failed.");

        auto io_context = std::make_shared<bzn::asio::io_context>();

        // setup signal handler...
        boost::asio::signal_set signals(io_context->get_io_context(), SIGINT, SIGTERM);

        signals.async_wait([io_context](const boost::system::error_code& error, int signal_number)
            {
                if (!error)
                {
                    LOG(info) << "signal received -- shutting down (" << signal_number << ")";
                    io_context->stop();
                }
            });

        // startup...
        auto crypto = std::make_shared<bzn::crypto>(options);
        auto chaos = std::make_shared<bzn::chaos>(io_context, options);
        auto websocket = std::make_shared<bzn::beast::websocket>();
        auto node = std::make_shared<bzn::node>(io_context, websocket, chaos, options->get_ws_idle_timeout(), boost::asio::ip::tcp::endpoint{options->get_listener()}, crypto, options);
        auto audit = std::make_shared<bzn::audit>(io_context, node, options->get_monitor_endpoint(io_context), options->get_uuid(), options->get_audit_mem_size(), options->pbft_enabled());
        std::shared_ptr<bzn::status> status;

        node->start();
        chaos->start();

        if (options->get_simple_options().get<bool>(bzn::option_names::AUDIT_ENABLED))
        {
            audit->start();
        }

        if (options->pbft_enabled())
        {
            auto failure_detector = std::make_shared<bzn::pbft_failure_detector>(io_context);

            // todo: for now use mem storage instead of rocksdb...
            auto unstable_storage = std::make_shared<bzn::mem_storage>();
            auto stable_storage = std::make_shared<bzn::mem_storage>();
            auto crud = std::make_shared<bzn::crud>(stable_storage, std::make_shared<bzn::subscription_manager>(io_context));

            auto pbft = std::make_shared<bzn::pbft>(node, io_context, peers.get_peers(), options->get_uuid(),
                std::make_shared<bzn::database_pbft_service>(io_context, unstable_storage, crud, options->get_uuid()), failure_detector);

            pbft->set_audit_enabled(options->get_simple_options().get<bool>(bzn::option_names::AUDIT_ENABLED));

            status = std::make_shared<bzn::status>(node, bzn::status::status_provider_list_t{pbft});

            crud->start();
            pbft->start();
            status->start();
        }
        else
        {
            // create http server using our configured listener address & http port number...
            auto ep = options->get_listener();
            ep.port(options->get_http_port());

            auto raft = std::make_shared<bzn::raft>(
                    io_context, node, peers.get_peers(),
                    options->get_uuid(), options->get_state_dir(), options->get_max_storage(),
                    options->peer_validation_enabled(), options->get_signed_key());

            // which type of storage?
            std::shared_ptr<bzn::storage_base> storage;

            if (options->get_mem_storage())
            {
                LOG(info) << "Using in-memory testing storage";
                storage = std::make_shared<bzn::mem_storage>();
            }
            else
            {
                LOG(info) << "Using RocksDB storage";
                storage = std::make_shared<bzn::rocksdb_storage>(options->get_state_dir(), options->get_uuid());
            }

            auto crud = std::make_shared<bzn::raft_crud>(node, raft, storage, std::make_shared<bzn::subscription_manager>(io_context));
            auto http_server = std::make_shared<bzn::http::server>(io_context, crud, ep);
            status = std::make_shared<bzn::status>(node, bzn::status::status_provider_list_t{raft});

            raft->set_audit_enabled(options->get_simple_options().get<bool>(bzn::option_names::AUDIT_ENABLED));

            raft->initialize_storage_from_log(storage);

            // These are here because they are not yet integrated with pbft
            http_server->start();
            crud->start();
            raft->start();
            status->start();
        }
        
        print_banner(*options, eth_balance);

        start_worker_threads_and_wait(io_context);
    }
    catch(std::exception& ex)
    {
        LOG(fatal) << ex.what();
        std::cerr << '\n' << ex.what() << '\n';
        return 1;
    }

    return 0;
}
