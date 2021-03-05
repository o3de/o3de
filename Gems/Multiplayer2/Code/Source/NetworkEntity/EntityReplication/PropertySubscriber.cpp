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

#include <Source/NetworkEntity/EntityReplication/PropertySubscriber.h>
#include <Source/NetworkEntity/EntityReplication/EntityReplicationManager.h>
#include <Source/Components/NetBindComponent.h>

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
