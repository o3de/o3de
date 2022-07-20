/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <Source/AutoGen/AutoComponentTypes.h>
#include <Source/Spawners/RoundRobinSpawner.h>
#include <Multiplayer/IMultiplayer.h>
#include <AzCore/Console/ILogger.h>

#include <AutomatedTestingSystemComponent.h>
#include <Multiplayer/ReplicationWindows/IReplicationWindow.h>

namespace AutomatedTesting
{
    void AutomatedTestingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AutomatedTestingSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AutomatedTestingSystemComponent>("AutomatedTesting", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AutomatedTestingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AutomatedTestingService"));
    }

    void AutomatedTestingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AutomatedTestingService"));
    }

    void AutomatedTestingSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("MultiplayerService"));
    }

    void AutomatedTestingSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AutomatedTestingSystemComponent::Init()
    {
    }

    void AutomatedTestingSystemComponent::Activate()
    {
        AutomatedTestingRequestBus::Handler::BusConnect();
        RegisterMultiplayerComponents(); //< Register AutomatedTesting's multiplayer components to assign NetComponentIds

        AZ::Interface<Multiplayer::IMultiplayerSpawner>::Register(this);
        m_playerSpawner = AZStd::make_unique<RoundRobinSpawner>();
        AZ::Interface<AutomatedTesting::IPlayerSpawner>::Register(m_playerSpawner.get());
    }

    void AutomatedTestingSystemComponent::Deactivate()
    {
        AZ::Interface<AutomatedTesting::IPlayerSpawner>::Unregister(m_playerSpawner.get());
        AZ::Interface<Multiplayer::IMultiplayerSpawner>::Unregister(this);
        AutomatedTestingRequestBus::Handler::BusDisconnect();
    }

    Multiplayer::NetworkEntityHandle AutomatedTestingSystemComponent::OnPlayerJoin([[maybe_unused]] uint64_t userId, [[maybe_unused]] const Multiplayer::MultiplayerAgentDatum& agentDatum)
    {
        AZStd::pair<Multiplayer::PrefabEntityId, AZ::Transform> entityParams = AZ::Interface<IPlayerSpawner>::Get()->GetNextPlayerSpawn();

        Multiplayer::INetworkEntityManager::EntityList entityList =
            AZ::Interface<Multiplayer::IMultiplayer>::Get()->GetNetworkEntityManager()->CreateEntitiesImmediate(
            entityParams.first, Multiplayer::NetEntityRole::Authority, entityParams.second, Multiplayer::AutoActivate::DoNotActivate);

        for (Multiplayer::NetworkEntityHandle subEntity : entityList)
        {
            subEntity.Activate();
        }

        Multiplayer::NetworkEntityHandle controlledEntity;
        if (!entityList.empty())
        {
            controlledEntity = entityList[0];
        }
        else
        {
            AZLOG_WARN("Attempt to spawn prefab %s failed. Check that prefab is network enabled.",
                entityParams.first.m_prefabName.GetCStr());
        }

        return controlledEntity;
    }

    void AutomatedTestingSystemComponent::OnPlayerLeave(Multiplayer::ConstNetworkEntityHandle entityHandle,
        [[maybe_unused]] const Multiplayer::ReplicationSet& replicationSet,
        [[maybe_unused]] AzNetworking::DisconnectReason reason)
    {
        AZ::Interface<Multiplayer::IMultiplayer>::Get()->GetNetworkEntityManager()->MarkForRemoval(entityHandle);
    }
}
