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

namespace Multiplayer
{
    inline NetEntityRole EntityReplicator::GetBoundLocalNetworkRole() const
    {
        return m_boundLocalNetworkRole;
    }

    inline NetEntityRole EntityReplicator::GetRemoteNetworkRole() const
    {
        return m_remoteNetworkRole;
    }

    inline ConstNetworkEntityHandle EntityReplicator::GetEntityHandle() const
    {
        return m_entityHandle;
    }

    inline NetBindComponent* EntityReplicator::GetNetBindComponent()
    {
        return m_netBindComponent;
    }

    inline const PrefabEntityId& EntityReplicator::GetPrefabEntityId() const
    {
        AZ_Assert(IsPrefabEntityIdSet(), "PrefabEntityId not set for Entity");
        return m_prefabEntityId;
    }

    inline bool EntityReplicator::IsPrefabEntityIdSet() const
    {
        return m_prefabEntityIdSet;
    }

    inline bool EntityReplicator::WasMigrated() const
    {
        return m_wasMigrated;
    }

    inline void EntityReplicator::SetWasMigrated(bool wasMigrated)
    {
        m_wasMigrated = wasMigrated;
    }

    inline PropertyPublisher* EntityReplicator::GetPropertyPublisher()
    {
        return m_propertyPublisher.get();
    }

    inline const PropertyPublisher* EntityReplicator::GetPropertyPublisher() const
    {
        return m_propertyPublisher.get();
    }

    inline PropertySubscriber* EntityReplicator::GetPropertySubscriber()
    {
        return m_propertySubscriber.get();
    }
}
