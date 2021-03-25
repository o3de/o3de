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

#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipService.h>
#include <AzToolsFramework/Prefab/PrefabLoader.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>


namespace AzToolsFramework
{
    PrefabEditorEntityOwnershipService::PrefabEditorEntityOwnershipService(const AzFramework::EntityContextId& entityContextId,
        AZ::SerializeContext* serializeContext)
        : m_entityContextId(entityContextId)
        , m_serializeContext(serializeContext)
    {
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            m_shouldAssertForLegacySlicesUsage, &AzToolsFramework::ToolsApplicationRequests::ShouldAssertForLegacySlicesUsage);
        AZ::Interface<PrefabEditorEntityOwnershipInterface>::Register(this);
    }

    PrefabEditorEntityOwnershipService::~PrefabEditorEntityOwnershipService()
    {
        AZ::Interface<PrefabEditorEntityOwnershipInterface>::Unregister(this);
    }

    void PrefabEditorEntityOwnershipService::Initialize()
    {
        m_prefabSystemComponent = AZ::Interface<Prefab::PrefabSystemComponentInterface>::Get();
        AZ_Assert(m_prefabSystemComponent != nullptr,
            "Couldn't get prefab system component, it's a requirement for PrefabEntityOwnership system to work");

        m_loaderInterface = AZ::Interface<Prefab::PrefabLoaderInterface>::Get();
        AZ_Assert(m_loaderInterface != nullptr,
            "Couldn't get prefab loader interface, it's a requirement for PrefabEntityOwnership system to work");

        m_rootInstance = AZStd::unique_ptr<Prefab::Instance>(m_prefabSystemComponent->CreatePrefab({}, {}, "/"));

        m_sliceOwnershipService.BusConnect(m_entityContextId);
        m_sliceOwnershipService.m_shouldAssertForLegacySlicesUsage = m_shouldAssertForLegacySlicesUsage;
        m_editorSliceOwnershipService.BusConnect();
        m_editorSliceOwnershipService.m_shouldAssertForLegacySlicesUsage = m_shouldAssertForLegacySlicesUsage;
    }

    bool PrefabEditorEntityOwnershipService::IsInitialized()
    {
        return m_rootInstance != nullptr;
    }

    void PrefabEditorEntityOwnershipService::Destroy()
    {
        m_editorSliceOwnershipService.BusDisconnect();
        m_sliceOwnershipService.BusDisconnect();
        m_rootInstance.reset();
    }

    void PrefabEditorEntityOwnershipService::Reset()
    {
        // TBD
    }

    void PrefabEditorEntityOwnershipService::AddEntity(AZ::Entity* entity)
    {
        AZ_Assert(IsInitialized(), "Tried to add an entity without initializing the Entity Ownership Service");
        m_rootInstance->AddEntity(*entity);
        HandleEntitiesAdded({ entity });
    }

    void PrefabEditorEntityOwnershipService::AddEntities(const EntityList& entities)
    {
        AZ_Assert(IsInitialized(), "Tried to add entities without initializing the Entity Ownership Service");
        for (AZ::Entity* entity : entities)
        {
            m_rootInstance->AddEntity(*entity);
        }
        HandleEntitiesAdded(entities);
    }

    bool PrefabEditorEntityOwnershipService::DestroyEntity(AZ::Entity* entity)
    {
        return DestroyEntityById(entity->GetId());
    }

    bool PrefabEditorEntityOwnershipService::DestroyEntityById(AZ::EntityId entityId)
    {
        AZ_Assert(IsInitialized(), "Tried to destroy an entity without initializing the Entity Ownership Service");
        AZ_Assert(m_entitiesRemovedCallback, "Callback function for DestroyEntityById has not been set.");
        AZStd::unique_ptr<AZ::Entity> detachedEntity = m_rootInstance->DetachEntity(entityId);
        if (detachedEntity)
        {
            AzFramework::SliceEntityRequestBus::MultiHandler::BusDisconnect(detachedEntity->GetId());
            m_entitiesRemovedCallback({ entityId });
            return true;
        }
        return false;
        // The detached entity now gets deleted because the unique_ptr gets out of scope
    }

    void PrefabEditorEntityOwnershipService::GetNonPrefabEntities(EntityList& entities)
    {
        m_rootInstance->GetEntities(entities, false);
    }

    bool PrefabEditorEntityOwnershipService::GetAllEntities(EntityList& entities)
    {
        m_rootInstance->GetEntities(entities, true);
        return true;
    }

    void PrefabEditorEntityOwnershipService::InstantiateAllPrefabs()
    {
    }

    void PrefabEditorEntityOwnershipService::HandleEntitiesAdded(const EntityList& entities)
    {
        AZ_Assert(m_entitiesAddedCallback, "Callback function for AddEntity has not been set.");
        for (const AZ::Entity* entity : entities)
        {
            AzFramework::SliceEntityRequestBus::MultiHandler::BusConnect(entity->GetId());
        }
        m_entitiesAddedCallback(entities);
    }

    bool PrefabEditorEntityOwnershipService::LoadFromStream(AZ::IO::GenericStream& /*stream*/, bool /*remapIds*/,
        EntityIdToEntityIdMap* /*idRemapTable*/, const AZ::ObjectStream::FilterDescriptor& /*filterDesc*/)
    {
        return true;
    }

    Prefab::InstanceOptionalReference PrefabEditorEntityOwnershipService::CreatePrefab(
        const AZStd::vector<AZ::Entity*>& entities, AZStd::vector<AZStd::unique_ptr<Prefab::Instance>>&& nestedPrefabInstances,
        const AZStd::string& filePath, Prefab::Instance& instanceToParentUnder)
    {
        AZStd::unique_ptr<Prefab::Instance> createdPrefabInstance =
            m_prefabSystemComponent->CreatePrefab(entities, AZStd::move(nestedPrefabInstances), filePath);

        if (createdPrefabInstance)
        {
            Prefab::Instance& addedInstance = instanceToParentUnder.AddInstance(AZStd::move(createdPrefabInstance));
            HandleEntitiesAdded({addedInstance.m_containerEntity.get()});
            return addedInstance;
        }
        return AZStd::nullopt;
    }

    Prefab::InstanceOptionalReference PrefabEditorEntityOwnershipService::GetRootPrefabInstance()
    {
        AZ_Assert(m_rootInstance, "A valid root prefab instance couldn't be found in PrefabEditorEntityOwnershipService.");
        return *m_rootInstance;
    }

    void PrefabEditorEntityOwnershipService::SetEntitiesAddedCallback(OnEntitiesAddedCallback onEntitiesAddedCallback)
    {
        m_entitiesAddedCallback = AZStd::move(onEntitiesAddedCallback);
    }

    void PrefabEditorEntityOwnershipService::SetEntitiesRemovedCallback(OnEntitiesRemovedCallback onEntitiesRemovedCallback)
    {
        m_entitiesRemovedCallback = AZStd::move(onEntitiesRemovedCallback);
    }

    void PrefabEditorEntityOwnershipService::SetValidateEntitiesCallback(ValidateEntitiesCallback validateEntitiesCallback)
    {
        m_validateEntitiesCallback = AZStd::move(validateEntitiesCallback);
    }

    //////////////////////////////////////////////////////////////////////////
    // Slice Buses implementation with Assert(false), this will exist only during Slice->Prefab
    // development to pinpoint and replace specific calls to Slice system

    AZ::SliceComponent::SliceInstanceAddress PrefabEditorEntityOwnershipService::GetOwningSlice()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AZ::SliceComponent::SliceInstanceAddress();
    }

   
    AZ::Data::AssetId UnimplementedSliceEntityOwnershipService::CurrentlyInstantiatingSlice()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    bool UnimplementedSliceEntityOwnershipService::HandleRootEntityReloadedFromStream(
        AZ::Entity*, bool, AZ::SliceComponent::EntityIdToEntityIdMap*)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return false;
    }

    AZ::SliceComponent* UnimplementedSliceEntityOwnershipService::GetRootSlice()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return nullptr;
    }

    const AZ::SliceComponent::EntityIdToEntityIdMap& UnimplementedSliceEntityOwnershipService::GetLoadedEntityIdMap()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        static AZ::SliceComponent::EntityIdToEntityIdMap dummy;
        return dummy;
    }

    AZ::EntityId UnimplementedSliceEntityOwnershipService::FindLoadedEntityIdMapping(const AZ::EntityId&) const
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AZ::EntityId();
    }

    AzFramework::SliceInstantiationTicket UnimplementedSliceEntityOwnershipService::InstantiateSlice(
        const AZ::Data::Asset<AZ::Data::AssetData>&,
        const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper&,
        const AZ::Data::AssetFilterCB&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AzFramework::SliceInstantiationTicket();
    }

    AZ::SliceComponent::SliceInstanceAddress UnimplementedSliceEntityOwnershipService::CloneSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress, AZ::SliceComponent::EntityIdToEntityIdMap&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    void UnimplementedSliceEntityOwnershipService::CancelSliceInstantiation(const AzFramework::SliceInstantiationTicket&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    AzFramework::SliceInstantiationTicket UnimplementedSliceEntityOwnershipService::GenerateSliceInstantiationTicket()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AzFramework::SliceInstantiationTicket();
    }

    void UnimplementedSliceEntityOwnershipService::SetIsDynamic(bool)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    const AzFramework::RootSliceAsset& UnimplementedSliceEntityOwnershipService::GetRootAsset() const
    {
        static AzFramework::RootSliceAsset dummy;
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return dummy;
    }

    //////////////////////////////////////////////////////////////////////////

    AzFramework::SliceInstantiationTicket UnimplementedSliceEditorEntityOwnershipService::InstantiateEditorSlice(
        const AZ::Data::Asset<AZ::Data::AssetData>&, const AZ::Transform&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return AzFramework::SliceInstantiationTicket();
    }

    AZ::SliceComponent::SliceInstanceAddress UnimplementedSliceEditorEntityOwnershipService::CloneEditorSliceInstance(
        AZ::SliceComponent::SliceInstanceAddress, AZ::SliceComponent::EntityIdToEntityIdMap&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    AZ::SliceComponent::SliceInstanceAddress UnimplementedSliceEditorEntityOwnershipService::CloneSubSliceInstance(
        const AZ::SliceComponent::SliceInstanceAddress&, const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>&,
        const AZ::SliceComponent::SliceInstanceAddress&, AZ::SliceComponent::EntityIdToEntityIdMap*)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    AZ::SliceComponent::SliceInstanceAddress UnimplementedSliceEditorEntityOwnershipService::PromoteEditorEntitiesIntoSlice(
        const AZ::Data::Asset<AZ::SliceAsset>&, const AZ::SliceComponent::EntityIdToEntityIdMap&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return {};
    }

    void UnimplementedSliceEditorEntityOwnershipService::DetachSliceEntities(const EntityIdList&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    void UnimplementedSliceEditorEntityOwnershipService::DetachSliceInstances(const AZ::SliceComponent::SliceInstanceAddressSet&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    void UnimplementedSliceEditorEntityOwnershipService::DetachSubsliceInstances(const AZ::SliceComponent::SliceInstanceEntityIdRemapList&)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    void UnimplementedSliceEditorEntityOwnershipService::RestoreSliceEntity(
        AZ::Entity*, const AZ::SliceComponent::EntityRestoreInfo&, SliceEntityRestoreType)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    void UnimplementedSliceEditorEntityOwnershipService::ResetEntitiesToSliceDefaults(EntityIdList)
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
    }

    AZ::SliceComponent* UnimplementedSliceEditorEntityOwnershipService::GetEditorRootSlice()
    {
        AZ_Assert(!m_shouldAssertForLegacySlicesUsage, "Slice usage with Prefab code enabled");
        return nullptr;
    }
}
