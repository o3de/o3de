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

#ifndef AZCORE_UNITTEST_NETBINDINGMOCKS_H
#define AZCORE_UNITTEST_NETBINDINGMOCKS_H

#include <AzTest/AzTest.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipServiceBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzFramework/Network/NetBindingHandlerBus.h>

namespace UnitTest
{
    class MockGameEntityContext
        : public AzFramework::GameEntityContextRequestBus::Handler
        , public AzFramework::SliceGameEntityOwnershipServiceRequestBus::Handler
    {
    public:
        MockGameEntityContext()
        {
            AzFramework::GameEntityContextRequestBus::Handler::BusConnect();
            AzFramework::SliceGameEntityOwnershipServiceRequestBus::Handler::BusConnect();
        }

        ~MockGameEntityContext()
        {
            AzFramework::SliceGameEntityOwnershipServiceRequestBus::Handler::BusDisconnect();
            AzFramework::GameEntityContextRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD3(InstantiateDynamicSlice, AzFramework::SliceInstantiationTicket(const AZ::Data::Asset<AZ::Data::AssetData>&, const AZ::Transform&, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper&));
        MOCK_METHOD0(GetGameEntityContextId, AzFramework::EntityContextId());
        MOCK_METHOD1(CreateGameEntity, AZ::Entity*(const char*));
        MOCK_METHOD1(AddGameEntity, void (AZ::Entity*));
        MOCK_METHOD1(DestroyGameEntity, void (const AZ::EntityId&));
        MOCK_METHOD1(DestroyGameEntityAndDescendants, void (const AZ::EntityId&));
        MOCK_METHOD1(ActivateGameEntity, void (const AZ::EntityId&));
        MOCK_METHOD1(DeactivateGameEntity, void (const AZ::EntityId&));
        MOCK_METHOD1(DestroyDynamicSliceByEntity, bool (const AZ::EntityId&));
        MOCK_METHOD2(LoadFromStream, bool (AZ::IO::GenericStream&, bool));
        MOCK_METHOD0(ResetGameContext, void ());
        MOCK_METHOD1(GetEntityName, AZStd::string (const AZ::EntityId&));
        MOCK_METHOD1(DestroySliceByEntity, bool(const AZ::EntityId&));
        MOCK_METHOD1(CreateGameEntityForBehaviorContext, AzFramework::BehaviorEntity (const char *));
        MOCK_METHOD1(CancelDynamicSliceInstantiation, void (const AzFramework::SliceInstantiationTicket &));
    };

    class MockNetBindingSystemContextData
        : public AzFramework::NetBindingSystemContextData
    {
    public:
        AZ_CLASS_ALLOCATOR(MockNetBindingSystemContextData, AZ::SystemAllocator, 0);

        static const char* GetChunkName()
        {
            return "MockNetBindingSystemContextData";
        }

        MOCK_METHOD1(OnAttachedToReplica, void (GridMate::Replica*));
        MOCK_METHOD1(OnDetachedFromReplica, void (GridMate::Replica*));
        MOCK_METHOD1(UpdateChunk, void (const GridMate::ReplicaContext&));
        MOCK_METHOD1(UpdateFromChunk, void (const GridMate::ReplicaContext&));
        MOCK_METHOD2(AcceptChangeOwnership, bool (GridMate::PeerId, const GridMate::ReplicaContext&));
        MOCK_METHOD1(OnReplicaChangeOwnership, void (const GridMate::ReplicaContext&));
        MOCK_METHOD0(IsUpdateFromReplicaEnabled, bool ());
        MOCK_CONST_METHOD1(ShouldSendToPeer, bool (GridMate::ReplicaPeer*));
        MOCK_METHOD1(CalculateDirtyDataSetMask, AZ::u32 (GridMate::MarshalContext&));
        MOCK_METHOD1(OnDataSetChanged, void (const GridMate::DataSetBase&));
        MOCK_METHOD2(Marshal, void (GridMate::MarshalContext&, AZ::u32));
        MOCK_METHOD2(Unmarshal, void (GridMate::UnmarshalContext&, AZ::u32));
        MOCK_METHOD0(IsReplicaMigratable, bool ());
        MOCK_METHOD0(IsBroadcast, bool ());
        MOCK_METHOD1(OnReplicaActivate, void (const GridMate::ReplicaContext&));
        MOCK_METHOD1(OnReplicaDeactivate, void (const GridMate::ReplicaContext&));

        /**
        * \brief Helper method for GoogleMock to call NetBindingSystemContextData::OnReplicaActivate
        */
        void Base_OnReplicaActivate(const GridMate::ReplicaContext& rc)
        {
            NetBindingSystemContextData::OnReplicaActivate(rc);
        }

        MOCK_METHOD0(GetReplicaManager, GridMate::ReplicaManager* ());
        MOCK_METHOD0(ShouldBindToNetwork, bool ());
    };

    class MockReplicaManager
        : public GridMate::ReplicaManager
    {
    public:
        MOCK_METHOD2(OnIncomingConnection, void (GridMate::Carrier*, GridMate::ConnectionID));
        MOCK_METHOD3(OnFailedToConnect, void (GridMate::Carrier*, GridMate::ConnectionID, GridMate::CarrierDisconnectReason));
        MOCK_METHOD3(OnDriverError, void (GridMate::Carrier*, GridMate::ConnectionID, const GridMate::DriverError&));
        MOCK_METHOD3(OnSecurityError, void (GridMate::Carrier*, GridMate::ConnectionID, const GridMate::SecurityError&));
        MOCK_METHOD1(Destroy, bool (GridMate::Replica*));
        MOCK_METHOD2(GetReplicaContext, void (const GridMate::Replica*, GridMate::ReplicaContext&));
        MOCK_METHOD2(OnConnectionEstablished, void (GridMate::Carrier*, GridMate::ConnectionID));
        MOCK_METHOD3(OnDisconnect, void (GridMate::Carrier*, GridMate::ConnectionID, GridMate::CarrierDisconnectReason));
        MOCK_METHOD3(OnRateChange, void (GridMate::Carrier*, GridMate::ConnectionID, AZ::u32));
        MOCK_METHOD1(FindReplica, GridMate::ReplicaPtr (GridMate::ReplicaId));
    };

    class MockAssetHandler
        : public AZ::Data::AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MockAssetHandler, AZ::SystemAllocator, 0)

        MOCK_METHOD2(CreateAsset, AZ::Data::AssetPtr (const AZ::Data::AssetId&, const AZ::Data::AssetType&));
        MOCK_METHOD3(LoadAssetData, AZ::Data::AssetHandler::LoadResult (
            const AZ::Data::Asset<AZ::Data::AssetData>&,
            AZStd::shared_ptr<AZ::Data::AssetDataStream>,
            const AZ::Data::AssetFilterCB&));
        MOCK_METHOD2(SaveAssetData, bool (const AZ::Data::Asset<AZ::Data::AssetData>&, AZ::IO::GenericStream*));
        MOCK_METHOD3(InitAsset, void (const AZ::Data::Asset<AZ::Data::AssetData>&, bool, bool));
        MOCK_METHOD1(DestroyAsset, void (AZ::Data::AssetPtr));
        MOCK_METHOD1(GetHandledAssetTypes, void (AZStd::vector<AZ::Data::AssetType>&));
        MOCK_CONST_METHOD1(CanHandleAsset, bool (const AZ::Data::AssetId&));
    };

    class MockAsset
        : public AZ::DynamicSliceAsset
    {
    public:
        AZ_RTTI(MockAsset, "{78ABC204-452E-4621-A552-F04D3ABF1690}", DynamicSliceAsset);

        MockAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId())
            : DynamicSliceAsset(assetId)
        {
        }

        ~MockAsset() = default;
    };

    class MockSliceReference
        : public AZ::SliceComponent::SliceReference
    {
    public:
        using SliceReference::SliceReference;

        MOCK_METHOD1(CreateInstance, AZ::SliceComponent::SliceInstance*(const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper&));
        MOCK_METHOD2(CloneInstance, AZ::SliceComponent::SliceInstance*(AZ::SliceComponent::SliceInstance*, AZ::SliceComponent::EntityIdToEntityIdMap&));
        MOCK_METHOD1(FindInstance, AZ::SliceComponent::SliceInstance*(const AZ::SliceComponent::SliceInstanceId&));
        MOCK_METHOD1(RemoveInstance, bool(AZ::SliceComponent::SliceInstance*));
        MOCK_METHOD3(RemoveEntity, bool(AZ::EntityId, bool, AZ::SliceComponent::SliceInstance*));
        MOCK_CONST_METHOD0(GetInstances, const AZ::SliceComponent::SliceReference::SliceInstances&());
        MOCK_CONST_METHOD0(GetSliceAsset, const AZ::Data::Asset<AZ::SliceAsset>& ());
        MOCK_CONST_METHOD0(GetSliceComponent, AZ::SliceComponent*());
        MOCK_CONST_METHOD0(IsInstantiated, bool ());
        MOCK_CONST_METHOD3(GetInstanceEntityAncestry, bool(const AZ::EntityId&, AZ::SliceComponent::EntityAncestorList&, AZ::u32));
        MOCK_METHOD0(ComputeDataPatch, void());
    };

    class MockSliceInstance
        : public AZ::SliceComponent::SliceInstance
    {
    public:
        using SliceInstance::SliceInstance;

        void SetMockInstantiatedContainer(AZ::SliceComponent::InstantiatedContainer* newContainer)
        {
            m_instantiated = newContainer;

            for (AZ::Entity* entity : m_instantiated->m_entities)
            {
                m_entityIdToBaseCache.insert(AZStd::make_pair(entity->GetId(), entity->GetId()));
            }

            for (AZ::Entity* entity : m_instantiated->m_entities)
            {
                m_baseToNewEntityIdMap.insert(AZStd::make_pair(entity->GetId(), entity->GetId()));
            }
        }

        MOCK_CONST_METHOD0(GetInstantiated, const AZ::SliceComponent::InstantiatedContainer*());
        MOCK_CONST_METHOD0(GetDataPatch, const AZ::DataPatch&());
        MOCK_CONST_METHOD0(GetDataFlags, const AZ::SliceComponent::DataFlagsPerEntity&());
        MOCK_METHOD0(GetDataFlags, AZ::SliceComponent::DataFlagsPerEntity&());
        MOCK_CONST_METHOD0(GetEntityIdMap, const AZ::SliceComponent::EntityIdToEntityIdMap& ());
        MOCK_CONST_METHOD0(GetEntityIdToBaseMap, const AZ::SliceComponent::EntityIdToEntityIdMap& ());
        MOCK_CONST_METHOD0(GetId, const AZ::SliceComponent::SliceInstanceId& ());
        MOCK_CONST_METHOD0(GetMetadataEntity, AZ::Entity* ());
    };

    class MockEntity
        : public AZ::Entity
    {
    public:
        ~MockEntity() override {}

        MOCK_METHOD0(Init, void ());
        MOCK_METHOD0(Activate, void ());
        MOCK_METHOD0(Deactivate, void ());

        /**
         * \brief Helper method for GoogleMock to call base class method
         */
        void Base_Init()
        {
            Entity::Init();
        }

        /**
         * \brief Helper method for GoogleMock to mark an entity as activated
         */
        void Base_Activate()
        {
            m_state = State::Active;
        }

        /**
        * \brief Helper method for GoogleMock to mark an entity as deactivated
        */
        void Base_Deactivate()
        {
            m_state = State::Init;
        }
    };

    class MockComponentApplication
        : public AZ::ComponentApplicationBus::Handler
    {
    public:
        MockComponentApplication()
        {
            AZ::ComponentApplicationBus::Handler::BusConnect();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
        }
        ~MockComponentApplication()
        {
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
            AZ::ComponentApplicationBus::Handler::BusDisconnect();
        }

        AZStd::vector<AZ::Entity*> m_mockEntities;

        bool AddEntity(AZ::Entity* entity) override
        {
            const auto it = AZStd::find(m_mockEntities.begin(), m_mockEntities.end(), entity);
            if (it == m_mockEntities.end())
            {
                m_mockEntities.push_back(entity);
                return true;
            }

            return false;
        }

        AZ::Entity* FindEntity(const AZ::EntityId& id) override
        {
            const auto it = AZStd::find_if(m_mockEntities.begin(), m_mockEntities.end(), [id](AZ::Entity* entity)
            {
                return entity->GetId() == id;
            });

            if (it != m_mockEntities.end())
            {
                return *it;
            }
            return nullptr;
        }

        MOCK_METHOD0(Destroy, void ());
        MOCK_METHOD1(RegisterComponentDescriptor, void (const AZ::ComponentDescriptor*));
        MOCK_METHOD1(UnregisterComponentDescriptor, void (const AZ::ComponentDescriptor*));
        MOCK_METHOD1(RegisterEntityAddedEventHandler, void(AZ::EntityAddedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityRemovedEventHandler, void(AZ::EntityRemovedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityActivatedEventHandler, void(AZ::EntityActivatedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityDeactivatedEventHandler, void(AZ::EntityDeactivatedEvent::Handler&));
        MOCK_METHOD1(SignalEntityActivated, void(AZ::Entity*));
        MOCK_METHOD1(SignalEntityDeactivated, void(AZ::Entity*));
        MOCK_METHOD1(RemoveEntity, bool (AZ::Entity*));
        MOCK_METHOD1(DeleteEntity, bool (const AZ::EntityId&));
        MOCK_METHOD1(GetEntityName, AZStd::string (const AZ::EntityId&));
        MOCK_METHOD1(EnumerateEntities, void (const ComponentApplicationRequests::EntityCallback&));
        MOCK_METHOD0(GetApplication, AZ::ComponentApplication* ());
        MOCK_METHOD0(GetSerializeContext, AZ::SerializeContext* ());
        MOCK_METHOD0(GetBehaviorContext, AZ::BehaviorContext* ());
        MOCK_METHOD0(GetJsonRegistrationContext, AZ::JsonRegistrationContext* ());
        MOCK_CONST_METHOD0(GetAppRoot, const char* ());
        MOCK_CONST_METHOD0(GetEngineRoot, const char* ());
        MOCK_CONST_METHOD0(GetExecutableFolder, const char* ());
        MOCK_METHOD0(GetDrillerManager, AZ::Debug::DrillerManager* ());
        MOCK_METHOD0(GetTickDeltaTime, float ());
        MOCK_METHOD0(GetTimeAtCurrentTick, AZ::ScriptTimePoint ());
        MOCK_METHOD1(Tick, void (float));
        MOCK_METHOD0(TickSystem, void ());
        MOCK_CONST_METHOD0(GetRequiredSystemComponents, AZ::ComponentTypeList ());
        MOCK_METHOD1(ResolveModulePath, void (AZ::OSString&));
        MOCK_METHOD0(RegisterCoreComponents, void ());
        MOCK_METHOD1(Reflect, void (AZ::ReflectContext*));
        MOCK_CONST_METHOD1(QueryApplicationType, void(AZ::ApplicationTypeQuery&));
    };

    class MockBindingComponent
        : public AZ::Component
        , public AzFramework::NetBindingHandlerBus::Handler
    {
    public:
        AZ_COMPONENT(MockBindingComponent, "{8393809A-3256-4865-97A9-1CCA43073B4A}", NetBindingHandlerInterface);

        static void Reflect(AZ::ReflectContext*) {}

        MOCK_METHOD0(Init, void ());
        MOCK_METHOD0(Activate, void ());
        MOCK_METHOD0(Deactivate, void ());
        MOCK_METHOD1(ReadInConfig, bool (const AZ::ComponentConfig*));
        MOCK_CONST_METHOD1(WriteOutConfig, bool (AZ::ComponentConfig*));
        MOCK_METHOD1(BindToNetwork, void (GridMate::ReplicaPtr));
        MOCK_METHOD0(UnbindFromNetwork, void ());
        MOCK_METHOD0(IsEntityBoundToNetwork, bool ());
        MOCK_METHOD0(IsEntityAuthoritative, bool ());
        MOCK_METHOD0(MarkAsLevelSliceEntity, void ());
        MOCK_METHOD1(SetSliceInstanceId, void (const AZ::SliceComponent::SliceInstanceId&));
        MOCK_METHOD1(SetReplicaPriority, void (GridMate::ReplicaPriority));
        MOCK_METHOD1(RequestEntityChangeOwnership, void (GridMate::PeerId));
        MOCK_CONST_METHOD0(GetReplicaPriority, GridMate::ReplicaPriority ());
    };
}

#endif // AZCORE_UNITTEST_NETBINDINGMOCKS_H
