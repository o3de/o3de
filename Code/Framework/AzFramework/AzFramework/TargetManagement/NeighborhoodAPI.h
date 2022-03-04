/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZFRAMEWORK_NEIGHBORHOODAPI_H
#define AZFRAMEWORK_NEIGHBORHOODAPI_H

/**
 * Neighborhood provides the API for apps to join and advertise their presence
 * to a particular development neighborhood.
 * A neighborhood is a peer-to-peer network of nodes (game, editor, hub, etc) so
 * that they can talk to each other during development.
 *
 * THIS FILE IS TO BE INCLUDED By THE TARGET MANAGER ONLY!!!
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace AzFrameworkPackets
{
    class Neighbor;
}

// neighborhood common
namespace Neighborhood {
    enum NeighborCapability
    {
        NEIGHBOR_CAP_NONE           = 0,
        NEIGHBOR_CAP_LUA_VM         = 1 << 0,
        NEIGHBOR_CAP_LUA_DEBUGGER   = 1 << 1,
    };
    AZ_TYPE_SAFE_INTEGRAL(NeighborCaps, uint32_t);

    /*
     * Neighborhood changes are broadcast at each node through this bus
     */
    class NeighborhoodEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~NeighborhoodEvents() {}

        // used to advertise a node and its capabilities
        virtual void OnNodeJoined([[maybe_unused]] const AzFrameworkPackets::Neighbor& node) {}
        // used to notify that a node is no longer available
        virtual void OnNodeLeft([[maybe_unused]] const AzFrameworkPackets::Neighbor& node) {}
    };
    typedef AZ::EBus<NeighborhoodEvents> NeighborhoodBus;

}   // namespace Neighborhood


#endif  // AZFRAMEWORK_NEIGHBORHOODAPI_H
#pragma once
