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

    NetworkSpawnableHolderComponent::NetworkSpawnableHolderComponent()
    {
    }

}
