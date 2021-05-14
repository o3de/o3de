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

#include <Source/NetworkTime/NetworkTime.h>
#include <Multiplayer/NetBindComponent.h>
#include <Multiplayer/IMultiplayer.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>

namespace Multiplayer
{
    NetworkTime::NetworkTime()
    {
        AZ::Interface<INetworkTime>::Register(this);
    }

    NetworkTime::~NetworkTime()
    {
        AZ::Interface<INetworkTime>::Unregister(this);
    }

    bool NetworkTime::IsTimeRewound() const
    {
        return m_rewindingConnectionId != AzNetworking::InvalidConnectionId;
    }

    HostFrameId NetworkTime::GetHostFrameId() const
    {
        return m_hostFrameId;
    }

    HostFrameId NetworkTime::GetUnalteredHostFrameId() const
    {
        return m_unalteredFrameId;
    }

    void NetworkTime::IncrementHostFrameId()
    {
        AZ_Assert(!IsTimeRewound(), "Incrementing the global application frameId is unsupported under a rewound time scope");
        ++m_unalteredFrameId;
        m_hostFrameId = m_unalteredFrameId;
    }

    AZ::TimeMs NetworkTime::GetHostTimeMs() const
    {
        return m_hostTimeMs;
    }

    AzNetworking::ConnectionId NetworkTime::GetRewindingConnectionId() const
    {
        return m_rewindingConnectionId;
    }

    HostFrameId NetworkTime::GetHostFrameIdForRewindingConnection(AzNetworking::ConnectionId rewindConnectionId) const
    {
        return (IsTimeRewound() && (rewindConnectionId == m_rewindingConnectionId)) ? m_unalteredFrameId : m_hostFrameId;
    }

    void NetworkTime::AlterTime(HostFrameId frameId, AZ::TimeMs timeMs, AzNetworking::ConnectionId rewindConnectionId)
    {
        m_hostFrameId = frameId;
        m_hostTimeMs = timeMs;
        m_rewindingConnectionId = rewindConnectionId;
    }

    void NetworkTime::SyncEntitiesToRewindState(const AZ::Aabb& rewindVolume)
    {
        // TODO: extrude rewind volume for initial gather
        AZStd::vector<AzFramework::VisibilityEntry*> gatheredEntries;
        AZ::Interface<AzFramework::IVisibilitySystem>::Get()->GetDefaultVisibilityScene()->Enumerate(rewindVolume, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData)
        {
            gatheredEntries.reserve(gatheredEntries.size() + nodeData.m_entries.size());
            for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
            {
                if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_Entity)
                {
                    // TODO: offset aabb for exact rewound position and check against the non-extruded rewind volume
                    gatheredEntries.push_back(visEntry);
                }
            }
        });

        for (AzFramework::VisibilityEntry* visEntry : gatheredEntries)
        {
            AZ::Entity* entity = static_cast<AZ::Entity*>(visEntry->m_userData);
            [[maybe_unused]] NetBindComponent* entryNetBindComponent = entity->template FindComponent<NetBindComponent>();
            if (entryNetBindComponent != nullptr)
            {
                // TODO: invoke the sync to rewind event on the netBindComponent and add the entity to the rewound entity set
            }
        }
    }

    void NetworkTime::ClearRewoundEntities()
    {
        AZ_Assert(!IsTimeRewound(), "Cannot clear rewound entity state while still within scoped rewind");
        // TODO: iterate all rewound entities, signal them to sync rewind state, and clear the rewound entity set
    }
}
