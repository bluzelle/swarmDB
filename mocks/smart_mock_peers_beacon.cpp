// Copyright (C) 2019 Bluzelle
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

#include <mocks/smart_mock_peers_beacon.hpp>

using namespace ::testing;

std::shared_ptr<bzn::mock_peers_beacon_base>
bzn::static_peers_beacon_for(bzn::peers_list_t peers)
{
    auto res = std::make_shared<bzn::mock_peers_beacon_base>();
    auto list = std::make_shared<bzn::peers_list_t>(peers);

    EXPECT_CALL(*res, start()).Times(AtMost(1));

    EXPECT_CALL(*res, current()).WillRepeatedly(Return(list));

    auto ordered = std::make_shared<ordered_peers_list_t>();
    std::for_each(peers.begin(), peers.end(),
            [&](const auto& peer)
            {
                ordered->push_back(peer);
            });

    std::sort(ordered->begin(), ordered->end(),
            [](const auto& peer1, const auto& peer2)
            {
                return peer1.uuid.compare(peer2.uuid) < 0;
            });

    EXPECT_CALL(*res, ordered()).WillRepeatedly(Return(ordered));

    EXPECT_CALL(*res, refresh(_)).Times(AnyNumber());

    return res;
}

std::shared_ptr<bzn::mock_peers_beacon_base>
bzn::static_peers_beacon_for(std::vector<bzn::peer_address_t> peers)
{
    bzn::peers_list_t list;
    std::for_each(peers.begin(), peers.end(),
            [&](const auto& peer){list.insert(peer);}
            );

    return static_peers_beacon_for(list);
}

std::shared_ptr<bzn::mock_peers_beacon_base>
bzn::static_empty_peers_beacon()
{
    return static_peers_beacon_for(std::vector<peer_address_t>{});
}
