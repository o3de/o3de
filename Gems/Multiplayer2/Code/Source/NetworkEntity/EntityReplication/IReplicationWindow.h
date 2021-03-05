/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <Source/MultiplayerTypes.h>
#include <Source/NetworkEntity/NetworkEntityHandle.h>
#include <AzCore/std/containers/map.h>

namespace Multiplayer
{
    class DebugScopeDrawMode;

    struct EntityReplicationData 
    {
        EntityReplicationData() = default;
        NetEntityRole m_networkRole = NetEntityRole::InvalidRole;
        float m_priority = 0.0f;
    };
    using ReplicationSet = AZStd::map<ConstNetworkEntityHandle, EntityReplicationData>;

    class IReplicationWindow
    {
    public:
        virtual ~IReplicationWindow() = default;
        virtual bool ReplicationSetUpdateReady() = 0;
        virtual const ReplicationSet& GetReplicationSet() const = 0;
        //! Max number of entities to track
        virtual uint32_t GetMaxEntityReplicatorCount() const = 0;
        //! Max number of entities we can send updates for in one frame
        virtual uint32_t GetMaxEntityReplicatorSendCount() const = 0;
        virtual bool IsInWindow(const ConstNetworkEntityHandle& entityPtr, NetEntityRole& outNetworkRole) const = 0;
        virtual void DebugDraw(DebugScopeDrawMode& drawMode) const = 0;
    };
}
