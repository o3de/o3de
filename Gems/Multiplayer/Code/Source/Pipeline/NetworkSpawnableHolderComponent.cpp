/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Pipeline/NetworkSpawnableHolderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Multiplayer/IMultiplayer.h>

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

    void NetworkSpawnableHolderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        // TransformService isn't strictly required in this component (Identity transform will be used by default)
        // However we need to make sure if there's a component providing TransformService it is activated first.
        dependent.push_back(AZ_CRC_CE("TransformService"));
    }

    NetworkSpawnableHolderComponent::NetworkSpawnableHolderComponent()
    {
    }

    void NetworkSpawnableHolderComponent::Activate()
    {
        IMultiplayer* multiplayer = GetMultiplayer();
        const bool shouldSpawnNetEntities = multiplayer->GetShouldSpawnNetworkEntities();

        if (shouldSpawnNetEntities)
        {
            AZ::Transform rootEntityTransform = AZ::Transform::CreateIdentity();

            if (auto* transformInterface = GetEntity()->GetTransform())
            {
                rootEntityTransform = transformInterface->GetWorldTM();
            }

            INetworkEntityManager* networkEntityManager = multiplayer->GetNetworkEntityManager();
            AZ_Assert(networkEntityManager != nullptr,
                "Network Entity Manager must be initialized before NetworkSpawnableHolderComponent is activated");

            m_netSpawnableTicket = networkEntityManager->RequestNetSpawnableInstantiation(m_networkSpawnableAsset, rootEntityTransform);
        }
    }

    void NetworkSpawnableHolderComponent::Deactivate()
    {
        m_netSpawnableTicket.reset();
    }

    void NetworkSpawnableHolderComponent::SetNetworkSpawnableAsset(AZ::Data::Asset<AzFramework::Spawnable> networkSpawnableAsset)
    {
        m_networkSpawnableAsset = networkSpawnableAsset;
    }

    AZ::Data::Asset<AzFramework::Spawnable> NetworkSpawnableHolderComponent::GetNetworkSpawnableAsset() const
    {
        return m_networkSpawnableAsset;
    }
}
