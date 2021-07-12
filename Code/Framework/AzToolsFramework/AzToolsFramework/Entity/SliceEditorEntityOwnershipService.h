/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/SliceEntityOwnershipService.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>

#include <QString>

namespace AzToolsFramework
{
    using EntityIdSet = AZStd::unordered_set<AZ::EntityId>;
    using EntityIdToEntityIdMap = AZStd::unordered_map<AZ::EntityId, AZ::EntityId>;

    class SliceEditorEntityOwnershipService
        : public AzFramework::SliceEntityOwnershipService
        , private SliceEditorEntityOwnershipServiceRequestBus::Handler
        , private AzFramework::SliceInstantiationResultBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceEditorEntityOwnershipService, AZ::SystemAllocator, 0);
        explicit SliceEditorEntityOwnershipService(const AzFramework::EntityContextId& entityContextId,
            AZ::SerializeContext* serializeContext);

        virtual ~SliceEditorEntityOwnershipService();

        /**
         * Switches to game mode in the editor.
         * 
         * \param m_editorToRuntimeIdMap The entityId map of editor entities to game entities.
         * \param m_runtimeToEditorIdMap The entityId map of game entities to editor entities.
         */
        void StartPlayInEditor(EntityIdToEntityIdMap& m_editorToRuntimeIdMap, EntityIdToEntityIdMap& m_runtimeToEditorIdMap);

        /**
         * Stops the game mode in the editor.
         * 
         * \param m_editorToRuntimeIdMap The entityId map of editor entities to game entities.
         * \param m_runtimeToEditorIdMap The entityId map of game entities to editor entities.
         */
        void StopPlayInEditor(EntityIdToEntityIdMap& m_editorToRuntimeIdMap, EntityIdToEntityIdMap& m_runtimeToEditorIdMap);

        /**
         * Saves the root slice of the entity ownership service to the specified buffer. Entities are saved as-is (with editor components).
         *
         * \param stream The stream to save the editor entity ownership service to.
         * \param entitiesInLayers A list of entities that were saved into layers.
         *        These won't be saved to the editor entity ownership service stream.
         * \param instancesInLayers The map of slice references to their instances that were saved into layers.
         *        These won't be saved to the editor entity ownership service stream.
         * \return true if successfully saved. Failure is only possible if serialization data is corrupt.
         */
        bool SaveToStreamForEditor(AZ::IO::GenericStream& stream, const EntityList& entitiesInLayers,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& instancesInLayers);

        /**
         * Saves the root slice of entity ownership serive to the specified buffer.
         * Entities undergo conversion for game: editor -> game components.
         *
         * \param stream The stream to save the entity ownership service to.
         * \param streamType
         * \return true if successfully saved. Failure is only possible if serialization data is corrupt.
         */
        bool SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType);

        /**
         * If the entities do not belong to a slice, associate them to the root slice metdata info component.
         * 
         * \param entities
         */
        void AssociateToRootMetadataEntity(const EntityList& entities);

        /**
         * Loads the root slice of EntityOwnershipService from the specified stream, and loads any layers referenced in the level.
         * 
         * \param stream The stream to load from.
         * \param levelPakFile The path to the level's pak file, which is used to find the layers.
         * \return true if successfully loaded.
         */
        bool LoadFromStreamWithLayers(AZ::IO::GenericStream& stream, QString levelPakFile);

        //////////////////////////////////////////////////////////////////////////
        // AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* context);

        using InstantiatingSlicePair = AZStd::pair<AZ::Data::Asset<AZ::Data::AssetData>, AZ::Transform>;
    protected:
        void HandleNewMetadataEntitiesCreated(AZ::SliceComponent& slice) override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus
        AzFramework::SliceInstantiationTicket InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset,
            const AZ::Transform& worldTransform) override;
        AZ::SliceComponent::SliceInstanceAddress CloneEditorSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance,
            AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) override;
        AZ::SliceComponent::SliceInstanceAddress CloneSubSliceInstance(
            const AZ::SliceComponent::SliceInstanceAddress& sourceSliceInstanceAddress,
            const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
            const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
            AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap) override;
        AZ::SliceComponent::SliceInstanceAddress PromoteEditorEntitiesIntoSlice(const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
            const AZ::SliceComponent::EntityIdToEntityIdMap& liveToAssetMap) override;
        void DetachSliceEntities(const EntityIdList& entities) override;
        void DetachSliceInstances(const AZ::SliceComponent::SliceInstanceAddressSet& instances) override;
        void DetachSubsliceInstances(const AZ::SliceComponent::SliceInstanceEntityIdRemapList& subsliceRootList) override;
        void RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info,
            SliceEntityRestoreType restoreType) override;
        void ResetEntitiesToSliceDefaults(EntityIdList entities) override;
        AZ::SliceComponent* GetEditorRootSlice() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::SliceInstantiationResultBus
        void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
        //////////////////////////////////////////////////////////////////////////

        void UpdateSelectedEntitiesInHierarchy(const EntityIdSet& entityIdSet);
        void DetachFromSlice(const EntityIdList& entities, const char* undoMessage);

        /**
         * Slice entity restore requests, which can be deferred if asset wasn't loaded at request time.
         */
        struct SliceEntityRestoreRequest
        {
            AZ::Entity* m_entity;
            AZ::SliceComponent::EntityRestoreInfo m_restoreInfo;
            AZ::Data::Asset<AZ::Data::AssetData> m_asset;
            SliceEntityRestoreType m_restoreType;
        };

        AZStd::vector<SliceEntityRestoreRequest> m_queuedSliceEntityRestores;

        AZStd::vector<InstantiatingSlicePair> m_instantiatingSlices;
    };
}

