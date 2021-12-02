/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/EntityReplication/PropertySubscriber.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicationManager.h>
#include <Multiplayer/Components/NetBindComponent.h>

namespace Multiplayer
{
    PropertySubscriber::PropertySubscriber(EntityReplicationManager& replicationManager, NetBindComponent* netBindComponent)
        : m_replicationManager(replicationManager)
        , m_netBindComponent(netBindComponent)
    {
        ;
    }

    AzNetworking::PacketId PropertySubscriber::GetLastReceivedPacketId() const
    {
        return m_lastReceivedPacketId;
    }

    bool PropertySubscriber::IsDeleting() const
    {
        return m_markForRemovalTimeMs > AZ::TimeMs{ 0 };
    }

    bool PropertySubscriber::IsDeleted() const
    {
        return m_markForRemovalTimeMs < m_replicationManager.GetFrameTimeMs();
    }

    void PropertySubscriber::SetDeleting()
    {
        m_markForRemovalTimeMs = AZ::TimeMs(m_replicationManager.GetFrameTimeMs() + m_replicationManager.GetResendTimeoutTimeMs());
    }

    bool PropertySubscriber::IsPacketIdValid(AzNetworking::PacketId packetId) const
    {
        return m_lastReceivedPacketId == AzNetworking::InvalidPacketId || packetId > m_lastReceivedPacketId;
    }

    bool PropertySubscriber::HandlePropertyChangeMessage(AzNetworking::PacketId packetId, AzNetworking::ISerializer* serializer, bool notifyChanges)
    {
        AZ_Assert(IsPacketIdValid(packetId), "Packet expected to be valid");
        m_lastReceivedPacketId = packetId;
        return m_netBindComponent->HandlePropertyChangeMessage(*serializer, notifyChanges);
    }
}
