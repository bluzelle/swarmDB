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

#include "raft_log.hpp"

#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <iostream>


namespace bzn
{
    raft_log::raft_log(const std::string& log_path, const size_t maximum_storage)
            :  maximum_storage(maximum_storage), entries_log_path(log_path)
    {
        if(boost::filesystem::exists(this->entries_log_path))
        {
            std::ifstream is(this->entries_log_path, std::ios::in | std::ios::binary);
            bzn::log_entry log_entry;
            while (is >> log_entry)
            {
                this->log_entries.emplace_back(log_entry);
            }
            this->total_memory_used = boost::filesystem::file_size(this->entries_log_path);
        }

        if (this->log_entries.empty())
        {
            throw std::runtime_error(MSG_ERROR_EMPTY_LOG_ENTRY_FILE);
        }
    }


    const bzn::log_entry&
    raft_log::entry_at(size_t i) const
    {
        return this->log_entries.at(i);
    }


    const bzn::log_entry&
    raft_log::last_quorum_entry() const
    {
        // TODO: Speed this up by not doing a search, when a quorum entry is added, simply store the index. Perhaps only do the search if the index is wrong.
        auto result = std::find_if(
            this->log_entries.crbegin(),
            this->log_entries.crend(),
            [](const auto &entry)
            {
                return (entry.entry_type == bzn::log_entry_type::single_quorum) ||
                       (entry.entry_type == bzn::log_entry_type::joint_quorum);
            });
        if (result == this->log_entries.crend())
        {
            throw std::runtime_error(MSG_NO_PEERS_IN_LOG);
        }
        return *result;
    }


    void
    raft_log::append_log_disk(const bzn::log_entry& log_entry)
    {
        if (!this->log_entry_out_stream.is_open())
        {
            boost::filesystem::path path{this->entries_log_path};
            if(!boost::filesystem::exists(path.parent_path()))
            {
                boost::system::error_code ec;
                if(!boost::filesystem::create_directories(path.parent_path(), ec))
                {
                    LOG(error) << "Unable to create path " << path.parent_path() << " with error code " << ec <<".";
                    throw std::runtime_error(MSG_EXITING_DUE_TO_LOG_PATH_CREATION_FAILURE);
                }
            }
            this->log_entry_out_stream.open(path.string(), std::ios::out |  std::ios::binary | std::ios::app);
        }
        this->log_entry_out_stream << log_entry;
        this->log_entry_out_stream.flush();
        this->total_memory_used = this->log_entry_out_stream.tellp();
    }


    void
    raft_log::leader_append_entry(const bzn::log_entry& log_entry)
    {
        LOG(debug) << "Appending " << log_entry_type_to_string(log_entry.entry_type) << " to my log: " << log_entry.msg.toStyledString();

        this->log_entries.emplace_back(log_entry);
        this->append_log_disk(log_entry);
    }


    void
    raft_log::follower_insert_entry(size_t index, const bzn::log_entry& log_entry)
    {
        // case 0: the index is in the log and we agree
        // case 1: the index is in the log and we disagree
        // case 2: the index is right after the log
        // case 3: the index is after the log

        if (index < this->log_entries.size() &&  log_entry.term == this->log_entries[index].term)
        {
            // we already have accepted this entry
            return;
        }

        if(index < this->log_entries.size() &&  log_entry.term != this->log_entries[index].term)
        {
            // throw away vector from index
            this->log_entries.erase( this->log_entries.begin() + index, this->log_entries.end());
            this->log_entries.emplace_back(log_entry);

            this->rewrite_log();
            this->append_log_disk(log_entry);
            return;
        }

        if(index == this->log_entries.size())
        {
            this->log_entries.emplace_back(log_entry);
            this->append_log_disk(log_entry);
            return;
        }

        if(index > this->log_entries.size())
        {
            throw std::runtime_error(MSG_TRYING_TO_INSERT_INVALID_ENTRY);
        }
    }


    bool
    raft_log::entry_accepted(size_t previous_index, size_t previous_term) const
    {
        if(previous_index < this->log_entries.size()-1)
        {
            return false;
        }
        return previous_term == this->log_entries[previous_index].term;
    }


    void
    raft_log::rewrite_log()
    {
        boost::filesystem::remove(this->entries_log_path);

        for(const auto& log_entry : this->log_entries)
        {
            this->append_log_disk(log_entry);
        }
    }


    size_t
    raft_log::size() const
    {
        return this->log_entries.size();
    }
}