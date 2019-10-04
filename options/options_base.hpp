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

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <include/bluzelle.hpp>
#include <include/boost_asio_beast.hpp>
#include <options/simple_options.hpp>
#include <string>
#include <map>
#include <optional>
#include <chrono>


namespace bzn
{
    // Suffixes for the max size parser.
    namespace utils
    {
        const std::map<char, size_t> BYTE_SUFFIXES = std::map<char, size_t>({{'B', 1},
                                                                             {'K', 1024},
                                                                             {'M', 1048576},
                                                                             {'G', 1073741824},
                                                                             {'T', 1099511627776}});
    }

    class options_base
    {
    public:
        virtual ~options_base() = default;

        /**
         * @return the raw_options container for accessing simple options
         */
        virtual const simple_options& get_simple_options() const = 0;


        /**
         * @return the raw_options container for accessing simple options
         */
        virtual simple_options& get_mutable_simple_options() = 0;


        /**
         * Get the address and port for the node to listen on
         * @return endpoint
         */
        virtual boost::asio::ip::tcp::endpoint get_listener() const = 0;


        /**
         * The address and port to send stats.d data to, if this is enabled
         * @return optional<endpoint>
         */
        virtual std::optional<boost::asio::ip::udp::endpoint> get_monitor_endpoint(
            std::shared_ptr<bzn::asio::io_context_base> context) const = 0;


        /**
         * Get the url to fetch initial peers from
         * @return url
         */
        virtual std::string get_bootstrap_peers_url() const = 0;


        /**
         * Get the file to fetch initial peers from
         * @return filename
         */
        virtual std::string get_bootstrap_peers_file() const = 0;


        /**
         * Debug logging level?
         * @return true if we should log debug entries
         */
        virtual bool get_debug_logging() const = 0;


        /**
         * Log to terminal instead of disk.
         * @return true if we log to stdout
         */
        virtual bool get_log_to_stdout() const = 0;


        /**
         * Get the peer's unique id
         * @return uuid
         */
        virtual bzn::uuid_t get_uuid() const = 0;


        /**
         * Get the swarm id this peer belongs to
         * @return swarm_id
         */
        virtual bzn::swarm_id_t get_swarm_id() const = 0;


        /**
         * Get the websocket activity timeout
         * @return seconds
         */
        virtual std::chrono::milliseconds get_ws_idle_timeout() const = 0;


        /**
         * Get the number of entries allowed to be stored in audit's datastructures
         * @return size
         */
        virtual size_t get_audit_mem_size() const = 0;


        /**
         * Get the directory for storing swarm state data
         * @return directory
         */
        virtual std::string get_state_dir() const = 0;


        /**
         * Get the director to log into
         * @return directory
         */
        virtual std::string get_logfile_dir() const = 0;


        /**
         * Get the maximum allowed storage for the daemon.
         * @return size
         */
        virtual size_t get_max_swarm_storage() const = 0;


        /**
         * Database to use
         * @return true if we are using in memory data
         */
        virtual bool get_mem_storage() const = 0;


        /**
         * Get the size of a log file to rotate
         * @return size
         */
        virtual size_t get_logfile_rotation_size() const = 0;


        /**
         * Get the total size of logs before deletion
         * @return size
         */
        virtual size_t get_logfile_max_size() const = 0;


         /**
          * Temporary toggle for the peer validation while in QA. Defaults to false.
          * @return boolean if the peer_validation member is set to true. Default is false.
          */
         virtual bool peer_validation_enabled() const = 0;


        /**
         * Signature for uuid signing verification
         * @return string containing the signature
         */
        virtual std::string get_signed_key() const = 0;


        /**
         * Retrieve the path to the Bluzelle public key pem file
         * @return string containing the path to the Bluzelle publi key pem file
         */
        virtual std::string get_owner_public_key() const = 0;


        /**
         * Retrieve the address of the ESR where the contract to return the swarm info is
         * @return string containing the address of the swarm info contract
         */
        virtual std::string get_swarm_info_esr_address() const = 0;


        /**
         * Retrieve the url of the server where the ESR is served
         * @return string containing the url of the server
         */
        virtual std::string get_swarm_info_esr_url() const = 0;

        /**
         * Retrieve the name of the stack the swarm is using
         * @return string stack name
         */
        virtual std::string get_stack() const = 0;

        /**
         * Get the number of entries allowed to be stored in audit's datastructures
         * @return size
         */
        virtual size_t get_admission_window() const = 0;
    };
} // bzn
