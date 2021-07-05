/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/MultiplayerTypes.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <AzCore/std/containers/map.h>

namespace Multiplayer
{
    struct EntityReplicationData 
    {
        EntityReplicationData() = default;
        NetEntityRole m_netEntityRole = NetEntityRole::InvalidRole;
        float m_priority = 0.0f;
    };
    using ReplicationSet = AZStd::map<ConstNetworkEntityHandle, EntityReplicationData>;

    class IReplicationWindow
    {
    public:
        virtual ~IReplicationWindow() = default;

        virtual bool ReplicationSetUpdateReady() = 0;
        virtual const ReplicationSet& GetReplicationSet() const = 0;
        //! Max number of entities we can send updates for in one frame
        virtual uint32_t GetMaxEntityReplicatorSendCount() const = 0;
        virtual bool IsInWindow(const ConstNetworkEntityHandle& entityPtr, NetEntityRole& outNetworkRole) const = 0;
        virtual void UpdateWindow() = 0;
        virtual void DebugDraw() const = 0;
    };
}
