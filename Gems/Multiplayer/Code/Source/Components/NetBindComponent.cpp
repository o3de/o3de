/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/MultiplayerComponent.h>
#include <Multiplayer/Components/MultiplayerController.h>
#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>
#include <Multiplayer/NetworkEntity/NetworkEntityRpcMessage.h>
#include <Multiplayer/NetworkEntity/NetworkEntityUpdateMessage.h>
#include <Multiplayer/NetworkInput/NetworkInput.h>
#include <Source/NetworkEntity/NetworkEntityTracker.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
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
                editContext->Class<NetBindComponent>(
                    "Network Binding", "The Network Binding component marks an entity as able to be replicated across the network")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/NetBind.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/NetBind.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<NetBindComponent>("NetBindComponent")
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Attribute(AZ::Script::Attributes::Category, "Multiplayer")

                ->Method("IsNetEntityRoleAuthority", [](AZ::EntityId id) -> bool {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning( "NetBindComponent", false, "NetBindComponent IsNetEntityRoleAuthority failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return false;
                    }

                    NetBindComponent* netBindComponent = GetNetworkEntityTracker()->GetNetBindComponent(entity);
                    if (!netBindComponent)
                    {
                        AZ_Warning( "NetBindComponent", false, "NetBindComponent IsNetEntityRoleAuthority failed. Entity '%s' (id: %s) is missing a NetBindComponent, make sure this entity contains a component which derives from NetBindComponent.", entity->GetName().c_str(), id.ToString().c_str())
                        return false;
                    }
                    return netBindComponent->IsNetEntityRoleAuthority();
                })

                ->Method("IsNetEntityRoleAutonomous", [](AZ::EntityId id) -> bool {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning( "NetBindComponent", false, "NetBindComponent IsNetEntityRoleAutonomous failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return false;
                    }

                    NetBindComponent* netBindComponent = GetNetworkEntityTracker()->GetNetBindComponent(entity);
                    if (!netBindComponent)
                    {
                        AZ_Warning("NetBindComponent", false, "NetBindComponent IsNetEntityRoleAutonomous failed. Entity '%s' (id: %s) is missing a NetBindComponent, make sure this entity contains a component which derives from NetBindComponent.", entity->GetName().c_str(), id.ToString().c_str())
                        return false;
                    }
                    return netBindComponent->IsNetEntityRoleAutonomous();
                })

                ->Method("IsNetEntityRoleClient", [](AZ::EntityId id) -> bool {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning( "NetBindComponent", false, "NetBindComponent IsNetEntityRoleClient failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return false;
                    }

                    NetBindComponent* netBindComponent = GetNetworkEntityTracker()->GetNetBindComponent(entity);
                    if (!netBindComponent)
                    {
                        AZ_Warning("NetBindComponent", false, "NetBindComponent IsNetEntityRoleClient failed. Entity '%s' (id: %s) is missing a NetBindComponent, make sure this entity contains a component which derives from NetBindComponent.", entity->GetName().c_str(), id.ToString().c_str())
                        return false;
                    }
                    return netBindComponent->IsNetEntityRoleClient();
                })

            ->Method("IsNetEntityRoleServer", [](AZ::EntityId id) -> bool {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning( "NetBindComponent", false, "NetBindComponent IsNetEntityRoleServer failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return false;
                    }

                    NetBindComponent* netBindComponent = GetNetworkEntityTracker()->GetNetBindComponent(entity);
                    if (!netBindComponent)
                    {
                        AZ_Warning("NetBindComponent", false, "NetBindComponent IsNetEntityRoleServer failed. Entity '%s' (id: %s) is missing a NetBindComponent, make sure this entity contains a component which derives from NetBindComponent.", entity->GetName().c_str(), id.ToString().c_str())
                        return false;
                    }
                    return netBindComponent->IsNetEntityRoleServer();
                })
            ;
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
        ;
    }

    void NetBindComponent::Activate()
    {
        m_needsToBeStopped = true;
        if (m_netEntityRole == NetEntityRole::Authority)
        {
            m_handleLocalServerRpcMessageEventHandle.Connect(m_sendServertoAuthorityRpcEvent);
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
        AZ_Assert(m_needsToBeStopped == false, "Entity appears to have been improperly deleted. Use MarkForRemoval to correctly clean up a networked entity.");
        m_handleLocalServerRpcMessageEventHandle.Disconnect();
        if (NetworkRoleHasController(m_netEntityRole))
        {
            GetNetworkEntityManager()->NotifyControllersDeactivated(m_netEntityHandle, EntityIsMigrating::False);
        }

        GetNetworkEntityTracker()->UnregisterNetBindComponent(this);
    }

    NetEntityRole NetBindComponent::GetNetEntityRole() const
    {
        return m_netEntityRole;
    }

    bool NetBindComponent::IsNetEntityRoleAuthority() const
    {
        return (m_netEntityRole == NetEntityRole::Authority);
    }

    bool NetBindComponent::IsNetEntityRoleAutonomous() const
    {
        return (m_netEntityRole == NetEntityRole::Autonomous)
            || (m_netEntityRole == NetEntityRole::Authority) && m_allowAutonomy;
    }

    bool NetBindComponent::IsNetEntityRoleServer() const
    {
        return (m_netEntityRole == NetEntityRole::Server);
    }

    bool NetBindComponent::IsNetEntityRoleClient() const
    {
        return (m_netEntityRole == NetEntityRole::Client);
    }

    bool NetBindComponent::HasController() const
    {
        return (m_netEntityRole == NetEntityRole::Authority)
            || (m_netEntityRole == NetEntityRole::Autonomous);
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

    void NetBindComponent::SetOwningConnectionId(AzNetworking::ConnectionId connectionId)
    {
        m_owningConnectionId = connectionId;
        for (MultiplayerComponent* multiplayerComponent : m_multiplayerInputComponentVector)
        {
            multiplayerComponent->SetOwningConnectionId(connectionId);
        }
    }

    AzNetworking::ConnectionId NetBindComponent::GetOwningConnectionId() const
    {
        return m_owningConnectionId;
    }

    void NetBindComponent::SetAllowAutonomy(bool value)
    {
        // This flag allows a player host to autonomously control their player entity, even though the entity is in an authority role
        m_allowAutonomy = value;
    }

    MultiplayerComponentInputVector NetBindComponent::AllocateComponentInputs()
    {
        MultiplayerComponentInputVector componentInputs;
        const size_t multiplayerComponentSize = m_multiplayerInputComponentVector.size();
        for (size_t i = 0; i < multiplayerComponentSize; ++i)
        {
            const NetComponentId netComponentId = m_multiplayerInputComponentVector[i]->GetNetComponentId();
            AZStd::unique_ptr<IMultiplayerComponentInput> componentInput = AZStd::move(GetMultiplayerComponentRegistry()->AllocateComponentInput(netComponentId));

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

    bool NetBindComponent::IsReprocessingInput() const
    {
        return m_isReprocessingInput;
    }

    void NetBindComponent::CreateInput(NetworkInput& networkInput, float deltaTime)
    {
        // Only autonomous runs this logic
        AZ_Assert(IsNetEntityRoleAutonomous(), "Incorrect network role for input creation");
        for (MultiplayerComponent* multiplayerComponent : m_multiplayerInputComponentVector)
        {
            multiplayerComponent->GetController()->CreateInputFromScript(networkInput, deltaTime);
            multiplayerComponent->GetController()->CreateInput(networkInput, deltaTime);
        }
    }

    void NetBindComponent::ProcessInput(NetworkInput& networkInput, float deltaTime)
    {
        m_isProcessingInput = true;
        // Only autonomous and authority runs this logic
        AZ_Assert((NetworkRoleHasController(m_netEntityRole)), "Incorrect network role for input processing");
        for (MultiplayerComponent* multiplayerComponent : m_multiplayerInputComponentVector)
        {
            multiplayerComponent->GetController()->ProcessInputFromScript(networkInput, deltaTime);
            multiplayerComponent->GetController()->ProcessInput(networkInput, deltaTime);
        }
        m_isProcessingInput = false;
    }

    void NetBindComponent::ReprocessInput(NetworkInput& networkInput, float deltaTime)
    {
        m_isReprocessingInput = true;
        ProcessInput(networkInput, deltaTime);
        m_isReprocessingInput = false;
    }

    bool NetBindComponent::HandleRpcMessage(AzNetworking::IConnection* invokingConnection, NetEntityRole remoteRole, NetworkEntityRpcMessage& message)
    {
        auto findIt = m_multiplayerComponentMap.find(message.GetComponentId());
        if (findIt != m_multiplayerComponentMap.end())
        {
            return findIt->second->HandleRpcMessage(invokingConnection, remoteRole, message);
        }
        return false;
    }

    bool NetBindComponent::HandlePropertyChangeMessage([[maybe_unused]] AzNetworking::ISerializer& serializer, [[maybe_unused]] bool notifyChanges)
    {
        const NetEntityRole netEntityRole = m_netEntityRole;
        ReplicationRecord replicationRecord(netEntityRole);
        replicationRecord.Serialize(serializer);
        if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && (netEntityRole == NetEntityRole::Server))
        {
            // Make sure to capture the entirety of the TotalRecord, before we clear out bits that haven't changed from our local state
            // If this entity migrates, we need to send all bits that might have changed from original baseline
            m_totalRecord.Append(replicationRecord);
        }
        // This will modify the replicationRecord and clear out bits that have not changed from the local state, this prevents us from notifying that something has changed multiple times
        SerializeStateDeltaMessage(replicationRecord, serializer);

        if (serializer.IsValid())
        {
            replicationRecord.ResetConsumedBits();
            if (notifyChanges)
            {
                NotifyStateDeltaChanges(replicationRecord);
            }

            // If we are deserializing on an entity, and this is a server simulation, then we need to remark our bits as dirty to replicate to the client
            if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && (netEntityRole == NetEntityRole::Server))
            {
                m_currentRecord.Append(replicationRecord);
                MarkDirty();
            }
        }
        return serializer.IsValid();
    }

    RpcSendEvent& NetBindComponent::GetSendAuthorityToClientRpcEvent()
    {
        return m_sendAuthorityToClientRpcEvent;
    }

    RpcSendEvent& NetBindComponent::GetSendAuthorityToAutonomousRpcEvent()
    {
        return m_sendAuthorityToAutonomousRpcEvent;
    }

    RpcSendEvent& NetBindComponent::GetSendServerToAuthorityRpcEvent()
    {
        return m_sendServertoAuthorityRpcEvent;
    }

    RpcSendEvent& NetBindComponent::GetSendAutonomousToAuthorityRpcEvent()
    {
        return m_sendAutonomousToAuthorityRpcEvent;
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
        NotifyStateDeltaChanges(m_localNotificationRecord);
        m_localNotificationRecord.Clear();
    }

    void NetBindComponent::NotifySyncRewindState()
    {
        m_syncRewindEvent.Signal();
    }

    void NetBindComponent::NotifyServerMigration(const HostId& remoteHostId)
    {
        m_entityServerMigrationEvent.Signal(m_netEntityHandle, remoteHostId);
    }

    void NetBindComponent::NotifyPreRender(float deltaTime)
    {
        m_entityPreRenderEvent.Signal(deltaTime);
    }

    void NetBindComponent::NotifyCorrection()
    {
        m_entityCorrectionEvent.Signal();
    }

    void NetBindComponent::AddEntityStopEventHandler(EntityStopEvent::Handler& eventHandler)
    {
        eventHandler.Connect(m_entityStopEvent);
    }

    void NetBindComponent::AddEntityDirtiedEventHandler(EntityDirtiedEvent::Handler& eventHandler)
    {
        eventHandler.Connect(m_dirtiedEvent);
    }

    void NetBindComponent::AddEntitySyncRewindEventHandler(EntitySyncRewindEvent::Handler& eventHandler)
    {
        eventHandler.Connect(m_syncRewindEvent);
    }

    void NetBindComponent::AddEntityServerMigrationEventHandler(EntityServerMigrationEvent::Handler& eventHandler)
    {
        eventHandler.Connect(m_entityServerMigrationEvent);
    }

    void NetBindComponent::AddEntityPreRenderEventHandler(EntityPreRenderEvent::Handler& eventHandler)
    {
        eventHandler.Connect(m_entityPreRenderEvent);
    }

    void NetBindComponent::AddEntityCorrectionEventHandler(EntityCorrectionEvent::Handler& eventHandler)
    {
        eventHandler.Connect(m_entityCorrectionEvent);
    }

    bool NetBindComponent::SerializeEntityCorrection(AzNetworking::ISerializer& serializer)
    {
        m_predictableRecord.ResetConsumedBits();
        ReplicationRecord tmpRecord = m_predictableRecord;
        // The m_predictableRecord is a record that that marks every NetworkProperty that has been set as Predictable
        // We copy this record and use a temporary so that SerializeStateDeltaMessage will not modify the m_predictableRecord
        // since SerializeStateDeltaMessage will clear the dirty bit for the NetworkProperty if it did not actually change
        const bool success = SerializeStateDeltaMessage(tmpRecord, serializer);
        if (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject)
        {
            tmpRecord.ResetConsumedBits();
            NotifyStateDeltaChanges(tmpRecord);
        }
        return success;
    }

    bool NetBindComponent::SerializeStateDeltaMessage(ReplicationRecord& replicationRecord, AzNetworking::ISerializer& serializer)
    {
        auto& stats = GetMultiplayer()->GetStats();
        stats.RecordEntitySerializeStart(serializer.GetSerializerMode(), GetEntityId(), GetEntity()->GetName().c_str());

        bool success = true;
        for (auto iter = m_multiplayerSerializationComponentVector.begin(); iter != m_multiplayerSerializationComponentVector.end(); ++iter)
        {
            success &= (*iter)->SerializeStateDeltaMessage(replicationRecord, serializer);

            stats.RecordComponentSerializeEnd(serializer.GetSerializerMode(), (*iter)->GetNetComponentId());
        }

        stats.RecordEntitySerializeStop(serializer.GetSerializerMode(), GetEntityId(), GetEntity()->GetName().c_str());

        return success;
    }

    void NetBindComponent::NotifyStateDeltaChanges(ReplicationRecord& replicationRecord)
    {
        for (auto iter = m_multiplayerSerializationComponentVector.begin(); iter != m_multiplayerSerializationComponentVector.end(); ++iter)
        {
            (*iter)->NotifyStateDeltaChanges(replicationRecord);
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

    void NetBindComponent::PreInit(AZ::Entity* entity, const PrefabEntityId& prefabEntityId, NetEntityId netEntityId, NetEntityRole netEntityRole)
    {
        AZ_Assert(entity != nullptr, "AZ::Entity is null");

        m_netEntityId = netEntityId;
        m_netEntityRole = netEntityRole;
        m_prefabEntityId = prefabEntityId;

        GetNetworkEntityTracker()->RegisterNetBindComponent(entity, this);

        m_netEntityHandle = GetNetworkEntityManager()->AddEntityToEntityMap(m_netEntityId, entity);

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
        case NetEntityRole::Client:
            m_netEntityRole = NetEntityRole::Autonomous;
            break;
        case NetEntityRole::Server:
            m_netEntityRole = NetEntityRole::Authority;
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
        case NetEntityRole::Autonomous:
            m_netEntityRole = NetEntityRole::Client;
            break;
        case NetEntityRole::Authority:
            m_netEntityRole = NetEntityRole::Server;
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
        if (GetNetEntityRole() == NetEntityRole::Authority)
        {
            m_handleLocalServerRpcMessageEventHandle.Connect(m_sendServertoAuthorityRpcEvent);
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
            component->NetworkAttach(this, m_currentRecord, m_predictableRecord);
        }
        m_totalRecord = m_currentRecord;
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
        message.SetRpcDeliveryType(RpcDeliveryType::ServerToAuthority);
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
                multiplayerComponent->SetOwningConnectionId(m_owningConnectionId);
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
        case NetEntityRole::Autonomous: // Fall through
        case NetEntityRole::Authority:
            return true;
        default:
            return false;
        }
    }
}
