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

#include <bootstrap/bootstrap_peers.hpp>
#include <crud/crud.hpp>
#include <node/node.hpp>
#include <options/options.hpp>
#include <raft/raft.hpp>
#include <storage/storage.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/program_options.hpp>

#include <thread>


void
init_logging()
{
    namespace keywords = boost::log::keywords;

    auto sink = boost::log::add_file_log
        (
            keywords::file_name = "bluzelle-%5N.log",
            keywords::rotation_size = 1024 * 64, // 64K logs
            keywords::open_mode = std::ios_base::app,
            keywords::auto_flush = true,
            keywords::format =
                (
                    boost::log::expressions::stream
                        << boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "[%Y-%m-%d %H:%M:%S.%f]")
                        << " [" << boost::log::expressions::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID")
                        << "] [" << std::setw(5) << std::left << boost::log::trivial::severity << "] " <<  boost::log::expressions::smessage
                )
        );

    boost::log::add_common_attributes();

    sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector
        (
            keywords::target = "logs/",
            keywords::max_size = 1024 * 512 // ~512K of logs
        ));

    sink->locked_backend()->scan_for_files();

    boost::log::core::get()->add_sink(sink);
}


void
set_logging_level(const bzn::options& options)
{
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



void
print_banner(const bzn::options& options)
{
    std::stringstream ss;

    ss  << "\nRunning node with ID: " /*<< daemon_info.id() */<< "\n"
        << " Ethereum Address ID: " << options.get_ethererum_address()  << "\n"
        << "    Local IP Address: " << options.get_listener().address().to_string() << "\n"
        << "             On port: " << options.get_listener().port()    << "\n"
        << "       Token Balance: " /*<< daemon_info.ropsten_token_balance() */<< " BLZ\n"
        << '\n';

    LOG(info) << ss.str();
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
        bzn::options options;

        if (!options.parse_command_line(argc, argv))
        {
            return 0;
        }

        if (!options.get_log_to_stdout())
        {
            init_logging();
        }

        set_logging_level(options);

        bzn::bootstrap_peers init_peers;
        std::string peers_file = options.get_bootstrap_peers_file();
        std::string peers_url = options.get_bootstrap_peers_url();

        if (peers_file.empty() && peers_url.empty())
        {
            LOG(error) << "Bootstrap peers must be specified options (bootstrap_file or bootstrap_url)";
            return 0;
        }

        if (!peers_file.empty())
        {
            init_peers.fetch_peers_from_file(peers_file);
        }

        if (!peers_url.empty())
        {
            init_peers.fetch_peers_from_url(peers_url);
        }

        if(init_peers.get_peers().empty())
        {
            LOG(error) << "Failed to find any bootstrap peers";
            return 0;
        }

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
        auto websocket = std::make_shared<bzn::beast::websocket>();

        auto node = std::make_shared<bzn::node>(io_context, websocket, boost::asio::ip::tcp::endpoint{options.get_listener()});
        auto raft = std::make_shared<bzn::raft>(io_context, node, init_peers.get_peers(), options.get_uuid());
        auto crud = std::make_shared<bzn::crud>(node, raft, std::make_shared<bzn::storage>());

        // todo: just for testing...
        node->register_for_message("ping",
            [](const bzn::message& msg, std::shared_ptr<bzn::session_base> session)
            {
                LOG(debug) << '\n' << msg.toStyledString();

                auto reply = std::make_shared<bzn::message>(msg);
                (*reply)["bzn-api"] = "pong";

                // echo back what the client sent...
                session->send_message(reply, nullptr);
            });

        node->start();
        crud->start();
        raft->start();

        print_banner(options);

        start_worker_threads_and_wait(io_context);
    }
    catch(std::exception& ex)
    {
        std::cerr << '\n' << ex.what() << '\n';
        return 1;
    }

    return 0;
}
