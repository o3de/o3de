/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/NetworkCommon.h>

namespace AzNetworking
{
    class ISerializer;
}

namespace Multiplayer
{
    class NetBindComponent;
    class EntityReplicationManager;

    class PropertySubscriber
    {
    public:
        PropertySubscriber(EntityReplicationManager& replicationManager, NetBindComponent* netBindComponent);

        bool IsDeleting() const;
        bool IsDeleted() const;
        void SetDeleting();

        bool IsPacketIdValid(AzNetworking::PacketId packetId) const;
        AzNetworking::PacketId GetLastReceivedPacketId() const;

        bool HandlePropertyChangeMessage(AzNetworking::PacketId packetId, AzNetworking::ISerializer* serializer, bool notifyChanges = true);

    private:
        EntityReplicationManager& m_replicationManager;
        NetBindComponent* m_netBindComponent;

        // The last packet to have been received about this entity
        AzNetworking::PacketId m_lastReceivedPacketId = AzNetworking::InvalidPacketId;
        AZ::TimeMs m_markForRemovalTimeMs = AZ::Time::ZeroTimeMs;
    };
}
