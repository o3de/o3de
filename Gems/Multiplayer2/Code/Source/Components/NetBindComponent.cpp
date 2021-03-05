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

#include <Source/Components/NetBindComponent.h>
#include <Source/Components/MultiplayerComponent.h>
#include <Source/Components/MultiplayerController.h>
#include <Source/NetworkEntity/INetworkEntityManager.h>
#include <Source/NetworkEntity/NetworkEntityRpcMessage.h>
#include <Source/NetworkEntity/NetworkEntityUpdateMessage.h>
#include <Source/NetworkInput/NetworkInput.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>

namespace Multiplayer
{
    void NetBindComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NetBindComponent, AZ::Component>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<NetBindComponent>("NetBindComponent", "Required Component for binding an entity to the network")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/NetBind.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/NetBind.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ;
            }
        }
    }

    void NetBindComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NetBindService"));
    }

    void NetBindComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NetBindService"));
    }

    NetBindComponent::NetBindComponent()
        : m_handleLocalServerRpcMessageEventHandle([this](NetworkEntityRpcMessage& message) { HandleLocalServerRpcMessage(message); })
        , m_handleMarkedDirty([this]() { HandleMarkedDirty(); })
        , m_handleNotifyChanges([this]() { NotifyLocalChanges(); })
        , m_handleEntityStateEvent([this](AZ::Entity::State oldState, AZ::Entity::State newState) { OnEntityStateEvent(oldState, newState); })
    {
        ;
    }

    void NetBindComponent::Init()
    {
        m_netEntityHandle = GetNetworkEntityManager()->AddEntityToEntityMap(m_netEntityId, GetEntity());
    }

    void NetBindComponent::Activate()
    {
        m_needsToBeStopped = true;
        if (m_netEntityRole == NetEntityRole::ServerAuthority)
        {
            m_handleLocalServerRpcMessageEventHandle.Connect(m_sendServerSimulationtoServerAuthorityRpcEvent);
        }
        if (NetworkRoleHasController(m_netEntityRole))
        {
            DetermineInputOrdering();
        }
        if (HasController())
        {
            // Listen for the entity to completely activate so that we can notify that all controllers have been activated
            GetEntity()->AddStateEventHandler(m_handleEntityStateEvent);
        }
    }

    void NetBindComponent::Deactivate()
    {
        AZ_Assert(m_needsToBeStopped == false, "Entity appears to have been deleted with using the EntityManagerBase.  Use MarkForRemoval to correctly clean up an entity");
        m_handleLocalServerRpcMessageEventHandle.Disconnect();
        if (NetworkRoleHasController(m_netEntityRole))
        {
            GetNetworkEntityManager()->NotifyControllersDeactivated(m_netEntityHandle, EntityIsMigrating::False);
        }
        NetworkDetach();
    }

    NetEntityRole NetBindComponent::GetNetEntityRole() const
    {
        return m_netEntityRole;
    }

    bool NetBindComponent::IsAuthority() const
    {
        return (m_netEntityRole == NetEntityRole::ServerAuthority);
    }

    bool NetBindComponent::HasController() const
    {
        return (m_netEntityRole == NetEntityRole::ServerAuthority)
            || (m_netEntityRole == NetEntityRole::ClientAutonomous);
    }

    NetEntityId NetBindComponent::GetNetEntityId() const
    {
        return m_netEntityId;
    }

    const PrefabEntityId& NetBindComponent::GetPrefabEntityId() const
    {
        return m_prefabEntityId;
    }

    ConstNetworkEntityHandle NetBindComponent::GetEntityHandle() const
    {
        return m_netEntityHandle;
    }

    NetworkEntityHandle NetBindComponent::GetEntityHandle()
    {
        return m_netEntityHandle;
    }

    MultiplayerComponentInputVector NetBindComponent::AllocateComponentInputs()
    {
        MultiplayerComponentInputVector componentInputs;
        for (MultiplayerComponent* multiplayerComponent : m_multiplayerInputComponentVector)
        {
            // TODO: ComponentInput factory, needs multiplayer component architecture and autogen
            AZStd::unique_ptr<IMultiplayerComponentInput> componentInput = nullptr; // ComponentInputFactory(multiplayerComponent->GetComponentId());
            if (componentInput != nullptr)
            {
                componentInputs.emplace_back(AZStd::move(componentInput));
            }
        }
        return componentInputs;
    }

    bool NetBindComponent::IsProcessingInput() const
    {
        return m_isProcessingInput;
    }

    void NetBindComponent::CreateInput(NetworkInput& networkInput, float deltaTime)
    {
        // Only autonomous or authority runs this logic
        AZ_Assert(m_netEntityRole == NetEntityRole::ClientAutonomous || m_netEntityRole == NetEntityRole::ServerAuthority, "Incorrect network role for input creation");
        for (MultiplayerComponent* multiplayerComponent : m_multiplayerInputComponentVector)
        {
            multiplayerComponent->GetController()->CreateInput(networkInput, deltaTime);
        }
    }

    void NetBindComponent::ProcessInput(NetworkInput& networkInput, float deltaTime)
    {
        // Only autonomous and authority runs this logic
        AZ_Assert((NetworkRoleHasController(m_netEntityRole)), "Incorrect network role for input processing");
        for (MultiplayerComponent* multiplayerComponent : m_multiplayerInputComponentVector)
        {
            multiplayerComponent->GetController()->ProcessInput(networkInput, deltaTime);
        }
    }

    float NetBindComponent::GetRewindDistanceForInput(const NetworkInput& networkInput, float deltaTime) const
    {
        AZ_Assert(m_netEntityRole == NetEntityRole::ServerAuthority, "Incorrect network role for computing rewind distance");
        float rewindDistance = 0.0f;
        for (MultiplayerComponent* multiplayerComponent : m_multiplayerInputComponentVector)
        {
            rewindDistance = AZStd::max(rewindDistance, multiplayerComponent->GetController()->GetRewindDistanceForInput(networkInput, deltaTime));
        }
        return rewindDistance;
    }

    bool NetBindComponent::HandleRpcMessage(NetEntityRole remoteRole, NetworkEntityRpcMessage& message)
    {
        auto findIt = m_multiplayerComponentMap.find(message.GetComponentId());
        if (findIt != m_multiplayerComponentMap.end())
        {
            return findIt->second->HandleRpcMessage(remoteRole, message);
        }
        return false;
    }

    bool NetBindComponent::HandlePropertyChangeMessage([[maybe_unused]] AzNetworking::ISerializer& serializer, [[maybe_unused]] bool notifyChanges)
    {
        const NetEntityRole netEntityRole = m_netEntityRole;
        ReplicationRecord replicationRecord(netEntityRole);
        replicationRecord.Serialize(serializer);
        if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && (netEntityRole == NetEntityRole::ServerSimulation))
        {
            // Make sure to capture the entirety of the TotalRecord, before we clear out bits that haven't changed from our local state
            // If this entity migrates, we need to send all bits that might have changed from original baseline
            m_totalRecord.Append(replicationRecord);
        }
        // This will modify the replicationRecord and clear out bits that have not changed from the local state, this prevents us from notifying that something has changed multiple times
        SerializeStateDeltaMessage(replicationRecord, serializer, ComponentSerializationType::Properties);

        if (serializer.IsValid())
        {
            replicationRecord.ResetConsumedBits();
            if (notifyChanges)
            {
                NotifyStateDeltaChanges(replicationRecord, ComponentSerializationType::Properties);
            }

            // If we are deserializing on an entity, and this is a server simulation, then we need to remark our bits as dirty to replicate to the client
            if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && (netEntityRole == NetEntityRole::ServerSimulation))
            {
                m_currentRecord.Append(replicationRecord);
                MarkDirty();
            }
        }
        return serializer.IsValid();
    }

    RpcSendEvent& NetBindComponent::GetSendServerAuthorityToClientSimulationRpcEvent()
    {
        return m_sendServerAuthorityToClientSimulationRpcEvent;
    }

    RpcSendEvent& NetBindComponent::GetSendServerAuthorityToClientAutonomousRpcEvent()
    {
        return m_sendServerAuthorityToClientAutonomousRpcEvent;
    }

    RpcSendEvent& NetBindComponent::GetSendServerSimulationToServerAuthorityRpcEvent()
    {
        return m_sendServerSimulationtoServerAuthorityRpcEvent;
    }

    RpcSendEvent& NetBindComponent::GetSendClientAutonomousToServerAuthorityRpcEvent()
    {
        return m_sendClientAutonomousToServerAuthorityRpcEvent;
    }

    const ReplicationRecord& NetBindComponent::GetPredictableRecord() const
    {
        return m_predictableRecord;
    }

    void NetBindComponent::MarkDirty()
    {
        if (!m_handleMarkedDirty.IsConnected())
        {
            GetNetworkEntityManager()->AddEntityMarkedDirtyHandler(m_handleMarkedDirty);
        }
    }

    void NetBindComponent::NotifyLocalChanges()
    {
        m_localNotificationRecord.ResetConsumedBits(); // Make sure our consumed bits are reset so that we can run through the notifications
        NotifyStateDeltaChanges(m_localNotificationRecord, ComponentSerializationType::Properties);
        m_localNotificationRecord.Clear();
    }

    void NetBindComponent::AddEntityStopEvent(EntityStopEvent::Handler& eventHandle)
    {
        eventHandle.Connect(m_entityStopEvent);
    }

    void NetBindComponent::AddEntityDirtiedEvent(EntityDirtiedEvent::Handler& eventHandle)
    {
        eventHandle.Connect(m_dirtiedEvent);
    }

    bool NetBindComponent::SerializeEntityCorrection(AzNetworking::ISerializer& serializer)
    {
        m_predictableRecord.ResetConsumedBits();
        ReplicationRecord tmpRecord = m_predictableRecord;
        // The m_predictableRecord is a record that that marks every NetworkProperty that has been set as Predictable
        // We copy this record and use a temporary so that SerializeStateDeltaMessage will not modify the m_predictableRecord
        // since SerializeStateDeltaMessage will clear the dirty bit for the NetworkProperty if it did not actually change
        const bool success = SerializeStateDeltaMessage(tmpRecord, serializer, ComponentSerializationType::Correction);
        if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
        {
            tmpRecord.ResetConsumedBits();
            NotifyStateDeltaChanges(tmpRecord, ComponentSerializationType::Correction);
        }
        return success;
    }

    bool NetBindComponent::SerializeStateDeltaMessage
    (
        ReplicationRecord& replicationRecord,
        AzNetworking::ISerializer& serializer,
        ComponentSerializationType componentSerializationType
    )
    {
        bool success = true;
        for (auto iter = m_multiplayerSerializationComponentVector.begin(); iter != m_multiplayerSerializationComponentVector.end(); ++iter)
        {
            success &= (*iter)->SerializeStateDeltaMessage(replicationRecord, serializer, componentSerializationType);
        }

        return success;
    }

    void NetBindComponent::NotifyStateDeltaChanges
    (
        ReplicationRecord& replicationRecord,
        ComponentSerializationType componentSerializationType
    )
    {
        for (auto iter = m_multiplayerSerializationComponentVector.begin(); iter != m_multiplayerSerializationComponentVector.end(); ++iter)
        {
            (*iter)->NotifyStateDeltaChanges(replicationRecord, componentSerializationType);
        }
    }

    void NetBindComponent::FillReplicationRecord(ReplicationRecord& replicationRecord) const
    {
        if (m_currentRecord.HasChanges())
        {
            replicationRecord.Append(m_currentRecord);
        }
    }

    void NetBindComponent::FillTotalReplicationRecord(ReplicationRecord& replicationRecord) const
    {
        replicationRecord.Append(m_totalRecord);
        // if we have any outstanding changes yet to be logged, grab those as well
        if (m_currentRecord.HasChanges())
        {
            replicationRecord.Append(m_currentRecord);
        }
    }

    bool NetBindComponent::IsMigrationDataValid() const
    {
        return m_isMigrationDataValid;
    }

    void NetBindComponent::SetMigrationDataValid(bool migrationDataValid)
    {
        m_isMigrationDataValid = migrationDataValid;
    }

    bool NetBindComponent::SerializeMigrationData(AzNetworking::ISerializer& serializer)
    {
        bool ret = true;
        // Purposefully not listed in reverse order, must match the order during construction
        for (auto iter = m_multiplayerSerializationComponentVector.begin(); iter != m_multiplayerSerializationComponentVector.end(); ++iter)
        {
            ret &= (*iter)->Migrate(serializer);
        }
        return ret && serializer.IsValid();
    }

    void NetBindComponent::PreInit(AZ::Entity* entity, const PrefabEntityId& prefabEntityId, NetEntityId netEntityId, NetEntityRole netEntityRole)
    {
        AZ_Assert(entity != nullptr, "AZ::Entity is null");

        m_prefabEntityId = prefabEntityId;
        m_netEntityId = netEntityId;
        m_netEntityRole = netEntityRole;

        for (AZ::Component* component : entity->GetComponents())
        {
            MultiplayerComponent* multiplayerComponent = azrtti_cast<MultiplayerComponent*>(component);
            if (multiplayerComponent != nullptr)
            {
                m_multiplayerComponentMap[multiplayerComponent->GetNetComponentId()] = multiplayerComponent;
            }
        }

        // Populate the component vector using component map ordering, since it's ordered by component type
        // It is absolutely essential that the ordering of this vector be consistent between client and server
        for (auto iter : m_multiplayerComponentMap)
        {
            m_multiplayerSerializationComponentVector.push_back(iter.second);
        }

        NetworkAttach();
    }

    void NetBindComponent::ConstructControllers()
    {
        switch (m_netEntityRole)
        {
        case NetEntityRole::ClientSimulation:
            m_netEntityRole = NetEntityRole::ClientAutonomous;
            break;
        case NetEntityRole::ServerSimulation:
            m_netEntityRole = NetEntityRole::ServerAuthority;
            break;
        default:
            AZ_Assert(false, "Controller already constructed");
        }

        // Use AZ component ordering to preserve component dependency ordering during controller construction
        const AZ::Entity::ComponentArrayType& entityComponents = GetEntity()->GetComponents();
        for (auto iter = entityComponents.begin(); iter != entityComponents.end(); ++iter)
        {
            MultiplayerComponent* multiplayerComponent = azrtti_cast<MultiplayerComponent*>(*iter);
            if (multiplayerComponent != nullptr)
            {
                multiplayerComponent->ConstructController();
            }
        }
    }

    void NetBindComponent::DestructControllers()
    {
        // Use AZ component ordering to preserve component dependency ordering during controller destruction
        const AZ::Entity::ComponentArrayType& entityComponents = GetEntity()->GetComponents();
        for (auto iter = entityComponents.rbegin(); iter != entityComponents.rend(); ++iter)
        {
            MultiplayerComponent* multiplayerComponent = azrtti_cast<MultiplayerComponent*>(*iter);
            if (multiplayerComponent != nullptr)
            {
                multiplayerComponent->DestructController();
            }
        }

        switch (m_netEntityRole)
        {
        case NetEntityRole::ClientAutonomous:
            m_netEntityRole = NetEntityRole::ClientSimulation;
            break;
        case NetEntityRole::ServerAuthority:
            m_netEntityRole = NetEntityRole::ServerSimulation;
            break;
        default:
            AZ_Assert(false, "Controllers already destructed");
        }
    }

    void NetBindComponent::ActivateControllers(EntityIsMigrating entityIsMigrating)
    {
        // Use AZ component ordering to preserve component dependency ordering
        const AZ::Entity::ComponentArrayType& entityComponents = GetEntity()->GetComponents();
        for (auto iter = entityComponents.begin(); iter != entityComponents.end(); ++iter)
        {
            MultiplayerComponent* multiplayerComponent = azrtti_cast<MultiplayerComponent*>(*iter);
            if (multiplayerComponent != nullptr)
            {
                multiplayerComponent->ActivateController(entityIsMigrating);
            }
        }
        DetermineInputOrdering();
        if (GetNetEntityRole() == NetEntityRole::ServerAuthority)
        {
            m_handleLocalServerRpcMessageEventHandle.Connect(m_sendServerSimulationtoServerAuthorityRpcEvent);
        }
        GetNetworkEntityManager()->NotifyControllersActivated(m_netEntityHandle, entityIsMigrating);
    }

    void NetBindComponent::DeactivateControllers(EntityIsMigrating entityIsMigrating)
    {
        m_handleLocalServerRpcMessageEventHandle.Disconnect();
        // Use AZ component ordering to preserve component dependency ordering
        const AZ::Entity::ComponentArrayType& entityComponents = GetEntity()->GetComponents();
        for (auto iter = entityComponents.rbegin(); iter != entityComponents.rend(); ++iter)
        {
            MultiplayerComponent* multiplayerComponent = azrtti_cast<MultiplayerComponent*>(*iter);
            if (multiplayerComponent != nullptr)
            {
                multiplayerComponent->DeactivateController(entityIsMigrating);
            }
        }
        GetNetworkEntityManager()->NotifyControllersDeactivated(m_netEntityHandle, entityIsMigrating);
    }

    void NetBindComponent::OnEntityStateEvent([[maybe_unused]] AZ::Entity::State oldState, AZ::Entity::State newState)
    {
        // Wait for the entity to change to an active state
        if (newState == AZ::Entity::State::Active)
        {
            GetNetworkEntityManager()->NotifyControllersActivated(m_netEntityHandle, EntityIsMigrating::False);
            m_handleEntityStateEvent.Disconnect();
        }
    }

    void NetBindComponent::NetworkAttach()
    {
        for (auto* component : m_multiplayerSerializationComponentVector)
        {
            component->NetworkAttach(*this, m_currentRecord, m_predictableRecord);
        }
        m_totalRecord = m_currentRecord;
    }

    void NetBindComponent::NetworkDetach()
    {
        for (auto* component : m_multiplayerSerializationComponentVector)
        {
            component->NetworkDetach();
        }
    }

    void NetBindComponent::HandleMarkedDirty()
    {
        m_dirtiedEvent.Signal();
        if (NetworkRoleHasController(GetNetEntityRole()))
        {
            m_localNotificationRecord.Append(m_currentRecord);
            if (!m_handleNotifyChanges.IsConnected())
            {
                GetNetworkEntityManager()->AddEntityNotifyChangesHandler(m_handleNotifyChanges);
            }
        }
        m_totalRecord.Append(m_currentRecord);
        m_currentRecord.Clear();
    }

    void NetBindComponent::HandleLocalServerRpcMessage(NetworkEntityRpcMessage& message)
    {
        message.SetRpcDeliveryType(RpcDeliveryType::ServerSimulationToServerAuthority);
        GetNetworkEntityManager()->HandleLocalRpcMessage(message);
    }

    void NetBindComponent::DetermineInputOrdering()
    {
        AZ_Assert(NetworkRoleHasController(m_netEntityRole), "Incorrect network role for input processing");

        m_multiplayerInputComponentVector.clear();
        // walk the components in the activation order so that our default ordering for input matches our dependency sort
        for (AZ::Component* component : GetEntity()->GetComponents())
        {
            MultiplayerComponent* multiplayerComponent = azrtti_cast<MultiplayerComponent*>(component);
            if (multiplayerComponent != nullptr)
            {
                m_multiplayerInputComponentVector.push_back(multiplayerComponent);
            }
        }
        AZStd::stable_sort
        (
            m_multiplayerInputComponentVector.begin(),
            m_multiplayerInputComponentVector.end(),
            [](MultiplayerComponent* left, MultiplayerComponent* right) -> bool
                {
                    return left->GetController()->GetInputOrder() < right->GetController()->GetInputOrder();
                }
        );
    }

    void NetBindComponent::StopEntity()
    {
        if (m_needsToBeStopped)
        {
            m_needsToBeStopped = false;
            m_entityStopEvent.Signal(m_netEntityHandle);
        }
    }

    bool NetworkRoleHasController(NetEntityRole networkRole)
    {
        switch (networkRole)
        {
        case NetEntityRole::ClientAutonomous: // Fall through
        case NetEntityRole::ServerAuthority:
            return true;
        default:
            return false;
        }
    }
}
