/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/PrefabEntityOwnershipService.h>
#include <AzFramework/Entity/SliceEntityOwnershipServiceBus.h>
#include <AzFramework/Slice/SliceEntityBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesContainer.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConversionPipeline.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class PrefabSystemComponentInterface;
        class PrefabLoaderInterface;
    }

    //////////////////////////////////////////////////////////////////////////
    // Implementation with Assert(false), this will exist only during Slice->Prefab
    // development to pinpoint and replace specific calls to Slice system

    class UnimplementedSliceEntityOwnershipService
        : public AzFramework::SliceEntityOwnershipServiceRequestBus::Handler
    {
    public:
        bool m_shouldAssertForLegacySlicesUsage = false;
    private:
        AZ::Data::AssetId CurrentlyInstantiatingSlice() override;

        bool HandleRootEntityReloadedFromStream(AZ::Entity* rootEntity, bool remapIds,
            AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable = nullptr) override;

        AZ::SliceComponent* GetRootSlice() override;

        const AZ::SliceComponent::EntityIdToEntityIdMap& GetLoadedEntityIdMap() override;

        AZ::EntityId FindLoadedEntityIdMapping(const AZ::EntityId& staticId) const override;

        AzFramework::SliceInstantiationTicket InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper = nullptr,
            const AZ::Data::AssetFilterCB& assetLoadFilter = nullptr) override;

        AZ::SliceComponent::SliceInstanceAddress CloneSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance,
            AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) override;

        void CancelSliceInstantiation(const AzFramework::SliceInstantiationTicket& ticket) override;
        AzFramework::SliceInstantiationTicket GenerateSliceInstantiationTicket() override;
        void SetIsDynamic(bool isDynamic) override;
        const AzFramework::RootSliceAsset& GetRootAsset() const override;
    };

    class UnimplementedSliceEditorEntityOwnershipService
        : public AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Handler
    {
    public:
        bool m_shouldAssertForLegacySlicesUsage = false;
    private:
        AzFramework::SliceInstantiationTicket InstantiateEditorSlice(
            const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset, const AZ::Transform& worldTransform) override;

        AZ::SliceComponent::SliceInstanceAddress CloneEditorSliceInstance(
            AZ::SliceComponent::SliceInstanceAddress sourceInstance,
            AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) override;

        AZ::SliceComponent::SliceInstanceAddress CloneSubSliceInstance(
            const AZ::SliceComponent::SliceInstanceAddress& sourceSliceInstanceAddress,
            const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
            const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
            AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap) override;

        AZ::SliceComponent::SliceInstanceAddress PromoteEditorEntitiesIntoSlice(
            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, const AZ::SliceComponent::EntityIdToEntityIdMap& liveToAssetMap) override;

        void DetachSliceEntities(const EntityIdList& entities) override;
        void DetachSliceInstances(const AZ::SliceComponent::SliceInstanceAddressSet& instances) override;
        void DetachSubsliceInstances(const AZ::SliceComponent::SliceInstanceEntityIdRemapList& subsliceRootList) override;
        void RestoreSliceEntity(
            AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info, SliceEntityRestoreType restoreType) override;
        void ResetEntitiesToSliceDefaults(EntityIdList entities) override;
        AZ::SliceComponent* GetEditorRootSlice() override;
    };
    //////////////////////////////////////////////////////////////////////////

    class PrefabEditorEntityOwnershipService
        : public AzFramework::PrefabEntityOwnershipService
        , private PrefabEditorEntityOwnershipInterface
        , private AzFramework::SliceEntityRequestBus::MultiHandler
    {
    public:
        using EntityList = AzFramework::EntityList;
        using OnEntitiesAddedCallback = AzFramework::OnEntitiesAddedCallback;
        using OnEntitiesRemovedCallback = AzFramework::OnEntitiesRemovedCallback;
        using ValidateEntitiesCallback = AzFramework::ValidateEntitiesCallback;

        static inline constexpr const char* DefaultMainSpawnableName = "Root";

        PrefabEditorEntityOwnershipService(
            const AzFramework::EntityContextId& entityContextId, AZ::SerializeContext* serializeContext);

        ~PrefabEditorEntityOwnershipService();

        //! Initializes all assets/entities/components required for managing entities.
        void Initialize() override;

        //! Returns true if the entity ownership service is initialized.
        bool IsInitialized() override;

        //! Destroys all the assets/entities/components created for managing entities.
        void Destroy() override;

        //! Resets the assets/entities/components without fully destroying them for managing entities.
        void Reset() override;

        void AddEntity(AZ::Entity* entity) override;

        void AddEntities(const EntityList& entities) override;

        bool DestroyEntity(AZ::Entity* entity) override;

        bool DestroyEntityById(const AZ::EntityId entityId) override;

        /**
        * Gets the entities in entity ownership service that do not belong to a prefab.
        *
        * \param entityList The entity list to add the entities to.
        */
        void GetNonPrefabEntities(EntityList& entities) override;

        /**
        * Gets all entities, including those that are owned by prefabs in the entity ownership service.
        *
        * \param entityList The entity list to add the entities to.
        * \return bool whether fetching entities was successful.
        */
        bool GetAllEntities(EntityList& entities) override;

        /**
        * Instantiates all the prefabs that are in the entity ownership service.
        */
        void InstantiateAllPrefabs() override;

        void HandleEntitiesAdded(const EntityList& entities) override;

        // To be removed in the future
        bool LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds,
            EntityIdToEntityIdMap* idRemapTable = nullptr,
            const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor()) override;

        void SetEntitiesAddedCallback(OnEntitiesAddedCallback onEntitiesAddedCallback) override;
        void SetEntitiesRemovedCallback(OnEntitiesRemovedCallback onEntitiesRemovedCallback) override;
        void SetValidateEntitiesCallback(ValidateEntitiesCallback validateEntitiesCallback) override;

        bool LoadFromStream(AZ::IO::GenericStream& stream, AZStd::string_view filename) override;
        bool SaveToStream(AZ::IO::GenericStream& stream, AZStd::string_view filename) override;
        
        void StartPlayInEditor() override;
        void StopPlayInEditor() override;

        void CreateNewLevelPrefab(AZStd::string_view filename, const AZStd::string& templateFilename) override;
        bool IsRootPrefabAssigned() const override;

    protected:

        AZ::SliceComponent::SliceInstanceAddress GetOwningSlice() override;

    private:
        struct PlayInEditorData
        {
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabConversionPipeline m_converter;
            AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> m_assets;
            AZStd::vector<AZ::Entity*> m_deactivatedEntities;
            AzFramework::SpawnableEntitiesContainer m_entities;
            bool m_isEnabled{ false };
        };
        PlayInEditorData m_playInEditorData;

        //////////////////////////////////////////////////////////////////////////
        // PrefabEditorEntityOwnershipInterface implementation
        Prefab::InstanceOptionalReference CreatePrefab(
            const AZStd::vector<AZ::Entity*>& entities, AZStd::vector<AZStd::unique_ptr<Prefab::Instance>>&& nestedPrefabInstances,
            AZ::IO::PathView filePath, Prefab::InstanceOptionalReference instanceToParentUnder) override;

        Prefab::InstanceOptionalReference InstantiatePrefab(
            AZ::IO::PathView filePath, Prefab::InstanceOptionalReference instanceToParentUnder) override;

        Prefab::InstanceOptionalReference GetRootPrefabInstance() override;
        Prefab::TemplateId GetRootPrefabTemplateId() override;
        
        const AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& GetPlayInEditorAssetData() override;
        //////////////////////////////////////////////////////////////////////////

        void OnEntityRemoved(AZ::EntityId entityId);

        void LoadReferencedAssets(AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets);

        OnEntitiesAddedCallback m_entitiesAddedCallback;
        OnEntitiesRemovedCallback m_entitiesRemovedCallback;
        ValidateEntitiesCallback m_validateEntitiesCallback;

        UnimplementedSliceEntityOwnershipService m_sliceOwnershipService;
        UnimplementedSliceEditorEntityOwnershipService m_editorSliceOwnershipService;

        AZStd::string m_rootPath;
        AZStd::unique_ptr<Prefab::Instance> m_rootInstance;
        Prefab::PrefabSystemComponentInterface* m_prefabSystemComponent;
        Prefab::PrefabLoaderInterface* m_loaderInterface;
        AzFramework::EntityContextId m_entityContextId;
        AZ::SerializeContext m_serializeContext;
        bool m_isRootPrefabAssigned = false;
    };
}
