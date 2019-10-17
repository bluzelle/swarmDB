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

#include <pbft/pbft_checkpoint_manager.hpp>
#include <proto/pbft.pb.h>
#include <utils/bytes_to_debug_string.hpp>
#include <utils/make_endpoint.hpp>
#include <algorithm>
#include <pbft/pbft.hpp>
#include <iterator>

namespace
{
    const std::chrono::seconds CHECKPOINT_CATCHUP_GRACE_PERIOD{std::chrono::seconds(30)};
}

using namespace bzn;

pbft_checkpoint_manager::pbft_checkpoint_manager(
        std::shared_ptr<asio::io_context_base> io_context,
        std::shared_ptr<storage_base> storage,
        std::shared_ptr<pbft_config_store> config_store,
        std::shared_ptr<node_base> node
)
        : io_context(std::move(io_context))
        , storage(std::move(storage))
        , config_store(std::move(config_store))
        , node(std::move(node))
{
    this->init_persists();
}

void
pbft_checkpoint_manager::start()
{
    this->node->register_for_message(bzn_envelope::kCheckpointMsg,
            std::bind(&pbft_checkpoint_manager::handle_checkpoint_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

checkpoint_t
pbft_checkpoint_manager::get_latest_stable_checkpoint() const
{
    std::lock_guard<std::mutex> lock(this->lock);
    return this->latest_stable_checkpoint.value();
}

checkpoint_t
pbft_checkpoint_manager::get_latest_local_checkpoint() const
{
    std::lock_guard<std::mutex> lock(this->lock);
    return this->latest_local_checkpoint.value();
}

void
pbft_checkpoint_manager::local_checkpoint_reached(const bzn::checkpoint_t& cp)
{
    std::lock_guard<std::mutex> lock(this->lock);

    LOG(info) << "reached local checkpoint at " << cp.first << " (" << bytes_to_debug_string(cp.second) << ")";
    if (cp.first <= this->latest_local_checkpoint.value().first)
    {
        throw std::runtime_error("reached a checkpoint at " + std::to_string(cp.first)
                                 + " which is not newer than the previous checkpoint at "
                                 + std::to_string(this->latest_local_checkpoint.value().first));
    }

    this->latest_local_checkpoint = cp;

    // Compose and send our own cp message
    checkpoint_msg cp_msg;
    cp_msg.set_sequence(cp.first);
    cp_msg.set_state_hash(cp.second);

    bzn_envelope msg;
    msg.set_checkpoint_msg(cp_msg.SerializeAsString());

    auto msg_ptr = std::make_shared<bzn_envelope>(msg);
    //msg_ptr->set_sender(this->options->get_uuid());
    for (const auto& peer : *(this->config_store->current()->get_peers()))
    {
        this->node->send_maybe_signed_message(make_endpoint(peer), msg_ptr);
    }

    if (cp.first == this->latest_local_checkpoint.value().first && cp.second != this->latest_local_checkpoint.value().second)
    {
        LOG(error) << "our checkpoint state disagrees with swarm! sending state request to reconcile";
        this->send_state_request();
    }

    // We will do the stabilization check upon receiving the message from ourself
}

void
pbft_checkpoint_manager::handle_checkpoint_message(const bzn_envelope& msg, std::shared_ptr<bzn::session_base> /*session*/)
{
    std::lock_guard<std::mutex> lock(this->lock);

    checkpoint_msg inner;
    if (!inner.ParseFromString(msg.checkpoint_msg()))
    {
        LOG(error) << "Failed to parse payload of checkpoint message: " << bytes_to_debug_string(msg.checkpoint_msg());
        return;
    }

    if (inner.sequence() < this->latest_local_checkpoint.value().first)
    {
        LOG(debug) << "Ignoring old checkpoint message at " << inner.sequence();
        return;
    }

    // check that sender is in current config

    LOG(debug) << boost::format("Got checkpoint message for seq %1% from %2%") % inner.sequence() % msg.sender();
    checkpoint_t cp(inner.sequence(), inner.state_hash());

    if (cp.first == this->latest_stable_checkpoint.value().first)
    {
        if (cp == this->latest_stable_checkpoint.value())
        {
            // save the message as more evidence on the current stable checkpoint (may matter as configs change)
            this->latest_stable_checkpoint_proof[msg.sender()] = persistent<std::string> {
                    this->storage, msg.SerializeAsString(), STABLE_CHECKPOINT_PROOF_KEY, msg.sender()
            };
        }
        else
        {
            LOG(error) << "Ignoring checkpoint message from " << msg.sender() << "; it conflicts with latest stable";
            return;
        }
    }
    else
    {
        if (this->partial_checkpoint_proofs[cp].count(msg.sender()) == 0)
        {
            this->partial_checkpoint_proofs[cp][msg.sender()] = persistent<std::string>{
                this->storage,
                msg.SerializeAsString(),
                PARTIAL_CHECKPOINT_PROOFS_KEY,
                cp,
                msg.sender()
            };
        }

        this->maybe_stabilize_checkpoint(cp);
    }
}

void
pbft_checkpoint_manager::maybe_stabilize_checkpoint(const checkpoint_t& cp)
{
    auto peers = this->config_store->current()->get_peers();

    // how many messages do we have supporting this checkpoint from peers in the current config?
    size_t checkpoint_attestants = std::count_if(this->partial_checkpoint_proofs[cp].begin(), this->partial_checkpoint_proofs[cp].end(),
            [&peers](const auto& pair)
            {
                return peers->end() != std::find_if(
                        peers->begin(), peers->end(), [&pair](const auto& peer)
                        {
                            return peer.uuid == pair.first;
                        }
                );
            }
    );

    if (checkpoint_attestants >= pbft::honest_member_size(peers->size()))
    {
        this->stabilize_checkpoint(cp);

        if (this->latest_local_checkpoint.value().first < cp.first)
        {
            LOG(info) << "We are behind the newly stable checkpoint; scheduling delayed state request";
            this->send_delayed_state_request(cp);
        }
        else if (this->latest_local_checkpoint.value().first == cp.first
                 && this->latest_stable_checkpoint.value().second != cp.second)
        {
            LOG(error) << "our checkpoint state disagrees with swarm! sending state request to reconcile";
            this->send_state_request();
        }
    }
}

void
pbft_checkpoint_manager::stabilize_checkpoint(const bzn::checkpoint_t& cp)
{
    LOG(info) << "Checkpoint at " << cp.first << " is now stable (" << bytes_to_debug_string(cp.second) << ")";

    // update the checkpoint
    this->latest_stable_checkpoint = cp;

    // clear the old proof
    for (auto& pair : this->latest_stable_checkpoint_proof)
    {
        pair.second.destroy();
    }
    this->latest_stable_checkpoint_proof.clear();

    // copy over the new proof
    for (const auto& pair : this->partial_checkpoint_proofs[cp])
    {
        this->latest_stable_checkpoint_proof[pair.first] = {this->storage, pair.second.value(), STABLE_CHECKPOINT_PROOF_KEY, pair.first};
    }

    // TODO: don't store incomplete proofs seperately

    // clear old data

    // for each partial proof
    for (auto& map_pair : this->partial_checkpoint_proofs)
    {
        const checkpoint_t& this_cp = map_pair.first;
        // if it's not newer than the newly stabilized proof
        if (this_cp.first > cp.first)
        {
            continue;
        }

        // delete all the entries
        for (auto& msg_pair : map_pair.second)
        {
            msg_pair.second.destroy();
        }
    }

    // now remove them from memory as well
    for (auto it = this->partial_checkpoint_proofs.begin(); it != this->partial_checkpoint_proofs.end();)
    {
        auto seq = it->first.first;
        if (seq > cp.first)
        {
            it++;
        }
        else
        {
            it = this->partial_checkpoint_proofs.erase(it);
        }
    }
}

void
pbft_checkpoint_manager::send_delayed_state_request(const checkpoint_t& cp)
{
    this->trigger_catchup_timer = this->io_context->make_unique_steady_timer();
    this->trigger_catchup_timer->expires_from_now(CHECKPOINT_CATCHUP_GRACE_PERIOD);
    this->trigger_catchup_timer->async_wait(
            [cp, weak_this = weak_from_this()](auto ec)
            {
                if (ec)
                {
                    return;
                }

                auto strong_this = weak_this.lock();

                // only request state if we are still behind the stable checkpoint
                if (strong_this && strong_this->latest_local_checkpoint.value().first < cp.first)
                {
                    std::lock_guard<std::mutex> lock(strong_this->lock);
                    strong_this->send_state_request();
                }
            }
    );
}

void
pbft_checkpoint_manager::send_state_request()
{
    pbft_membership_msg msg;
    msg.set_type(PBFT_MMSG_GET_STATE);
    msg.set_sequence(this->latest_stable_checkpoint.value().first);
    msg.set_state_hash(this->latest_stable_checkpoint.value().second);

    uint32_t selected = pbft::generate_random_number(0, this->latest_stable_checkpoint_proof.size() - 1);
    auto it = this->latest_stable_checkpoint_proof.begin();
    std::advance(it, selected);
    auto selected_peer = (*it).first;

    LOG(info) << "Requesting state at " << msg.sequence() << " from " << selected_peer;

    auto msg_ptr = std::make_shared<bzn_envelope>();
    msg_ptr->set_pbft_membership(msg.SerializeAsString());

    this->node->send_maybe_signed_message(selected_peer, msg_ptr);
}

void
pbft_checkpoint_manager::init_persists()
{
    persistent<std::string>::init_kv_container<bzn::uuid_t>(this->storage, STABLE_CHECKPOINT_PROOF_KEY,
            this->latest_stable_checkpoint_proof);
    persistent<std::string>::init_kv_container2<bzn::uuid_t, checkpoint_t>(this->storage, PARTIAL_CHECKPOINT_PROOFS_KEY,
            this->partial_checkpoint_proofs);
}

std::unordered_map<bzn::uuid_t, std::string>
pbft_checkpoint_manager::get_latest_stable_checkpoint_proof() const
{
    std::lock_guard<std::mutex> lock(this->lock);
    std::unordered_map<bzn::uuid_t, std::string> result;
    for (const auto& pair : this->latest_stable_checkpoint_proof)
    {
        result[pair.first] = pair.second.value();
    }

    return result;
}

size_t
pbft_checkpoint_manager::partial_checkpoint_proofs_count() const
{
    std::lock_guard<std::mutex> lock(this->lock);
    return this->partial_checkpoint_proofs.size();
}
