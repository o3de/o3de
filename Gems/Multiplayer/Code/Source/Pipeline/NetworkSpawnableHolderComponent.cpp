/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Pipeline/NetworkSpawnableHolderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Multiplayer
{
    void NetworkSpawnableHolderComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetworkSpawnableHolderComponent, AZ::Component>()
                ->Version(1)
                ->Field("AssetRef", &NetworkSpawnableHolderComponent::m_networkSpawnableAsset);
        }
    }

    NetworkSpawnableHolderComponent::NetworkSpawnableHolderComponent()
    {
    }

    void NetworkSpawnableHolderComponent::Activate()
    {
    }

    void NetworkSpawnableHolderComponent::Deactivate()
    {
    }

    void NetworkSpawnableHolderComponent::SetNetworkSpawnableAsset(AZ::Data::Asset<AzFramework::Spawnable> networkSpawnableAsset)
    {
        m_networkSpawnableAsset = networkSpawnableAsset;
    }

    AZ::Data::Asset<AzFramework::Spawnable> NetworkSpawnableHolderComponent::GetNetworkSpawnableAsset()
    {
        return m_networkSpawnableAsset;
    }
}
