/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>

#include <AzFramework/Entity/EntityOwnershipService.h>
#include <AzFramework/Entity/SliceEntityOwnershipServiceBus.h>
#include <AzFramework/Slice/SliceEntityBus.h>

namespace AzFramework
{
    using SliceInstanceUnorderedSet = AZStd::unordered_set<AZ::SliceComponent::SliceInstanceAddress>;

    /**
     * SliceEntityOwnershipService uses slices as the prefab mechanism to manage entities used by an entity context.
     * This includes using a root-slice to put all the loose entities in a level that don't belong to any layers or slices.
     */
    class SliceEntityOwnershipService
        : public EntityOwnershipService
        , protected SliceEntityOwnershipServiceRequestBus::Handler
        , private SliceEntityRequestBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceEntityOwnershipService, AZ::SystemAllocator);

        explicit SliceEntityOwnershipService(const EntityContextId& entityContextId, AZ::SerializeContext* serializeContext);

        //////////////////////////////////////////////////////////////////////////
        // SliceEntityOwnershipService

        //! Creates the root-slice asset under which all entities in the level belong.
        void Initialize() override;

        //! Returns true if root slice asset is present.
        bool IsInitialized() override;

        //! Destroys the root-slice asset.
        void Destroy() override;

        //! Destroys all the entities under the root-slice without destroying it fully.
        void Reset() override;

        //! Adds an entity to the root-slice.
        //! @param entity
        void AddEntity(AZ::Entity* entity) override;

        //! Adds the given entities to the root slice.
        //! @param entities
        void AddEntities(const EntityList& entities) override;

        //! Deletes the entity from the root-slice and destroys it.
        //! @param entity
        bool DestroyEntity(AZ::Entity* entity) override;

        //! Deletes the entity id from the root-slice and destroys it.
        //! @param entityId
        bool DestroyEntityById(AZ::EntityId entityId) override;

        //! Gets the entities in entity ownership service that do not belong to a prefab.
        void GetNonPrefabEntities(EntityList& entityList) override;
        
        //! Gets all entities, including those that are owned by prefabs in the entity ownership service.
        //! @param entityList The entity list to add the entities to.
        //! @return whether fetching entities was successful.
        bool GetAllEntities(EntityList& entityList) override;

        //! Instantiates all the prefabs that are in the entity ownership service.
        void InstantiateAllPrefabs() override;

        void SetEntitiesAddedCallback(OnEntitiesAddedCallback onEntitiesAddedCallback) override;
        void SetEntitiesRemovedCallback(OnEntitiesRemovedCallback onEntityRemovedCallback) override;
        void SetValidateEntitiesCallback(ValidateEntitiesCallback validateEntitiesCallback) override;

        void HandleEntityBeingDestroyed(const AZ::EntityId& entityId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetBus
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        //! Load the root slice from a stream.
        //! @param stream the source stream from which to load
        //! @param remapIds if true, entity Ids will be remapped post-load
        //! @param idRemapTable if remapIds is true, the provided table is filled with a map of original ids to new ids
        //! @param filterDesc any ObjectStream::LoadFlags
        //! @return whether or not the root slice was successfully loaded from the provided stream
        bool LoadFromStream(AZ::IO::GenericStream& stream, bool remapIds,
            EntityIdToEntityIdMap* idRemapTable = nullptr,
            const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor()) override;

        //! Executes the post-add actions for the provided list of entities, like connecting to required ebuses.
        //! @param entities The entities to perform the post-add actions for.
        void HandleEntitiesAdded(const EntityList& entities) override;

        static void Reflect(AZ::ReflectContext* context);

    protected:

        //////////////////////////////////////////////////////////////////////////
        // SliceEntityRequestBus
        AZ::SliceComponent::SliceInstanceAddress GetOwningSlice() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SliceEntityOwnershipServiceRequestBus
        AZ::Data::AssetId CurrentlyInstantiatingSlice() override;
        bool HandleRootEntityReloadedFromStream(AZ::Entity* rootEntity, bool remapIds,
            AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable = nullptr) override;

        AZ::SliceComponent* GetRootSlice() override;
        const AZ::SliceComponent::EntityIdToEntityIdMap& GetLoadedEntityIdMap() override;
        AZ::EntityId FindLoadedEntityIdMapping(const AZ::EntityId& staticId) const override;
        SliceInstantiationTicket InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper = nullptr,
            const AZ::Data::AssetFilterCB& assetLoadFilter = nullptr) override;

        AZ::SliceComponent::SliceInstanceAddress CloneSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance,
            AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) override;

        void CancelSliceInstantiation(const SliceInstantiationTicket& ticket) override;
        SliceInstantiationTicket GenerateSliceInstantiationTicket() override;
        void SetIsDynamic(bool isDynamic) override;
        const RootSliceAsset& GetRootAsset() const override;
        //////////////////////////////////////////////////////////////////////////

        AZ::SliceComponent::SliceInstanceAddress GetOwningSlice(AZ::EntityId entityId);
        AZ::SerializeContext* GetSerializeContext();
        virtual void CreateRootSlice();
        void CreateRootSlice(AZ::SliceAsset* rootSliceAsset);

        //! Properly process new metadata entities created during slice streaming. Because the streaming process bypasses slice creation,
        //! the entity context has to make sure they're handled properly.
        //! @param slice - The slice that was streamed in
        virtual void HandleNewMetadataEntitiesCreated(AZ::SliceComponent& /*slice*/) {};

    private:
        EntityList GetRootSliceEntities();

        // Checks if the root and root component is available, asserts if its not, and returns true if things are OK to proceed, false otherwise.
        bool CheckAndAssertRootComponentIsAvailable();

        /// Helper function to send OnSliceInstantiationFailed events.
        static void DispatchOnSliceInstantiationFailed(const SliceInstantiationTicket& ticket,
            const AZ::Data::AssetId& assetId, bool canceled);

        /// Tracking of pending slice instantiations, each being the requested asset and the associated request's ticket.
        struct InstantiatingSliceInfo
        {
            InstantiatingSliceInfo(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const SliceInstantiationTicket& ticket,
                const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customMapper)
                : m_asset(asset)
                , m_ticket(ticket)
                , m_customMapper(customMapper)
            {
            }

            AZ::Data::Asset<AZ::Data::AssetData>            m_asset;
            SliceInstantiationTicket                        m_ticket;
            AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper   m_customMapper;
        };

        //! Slices queued for instantation. AZStd::list is used for its stable iterators since elements
        //! are deleted during traversal in SliceEntityOwnershipService::OnAssetReady
        AZStd::list<InstantiatingSliceInfo> m_queuedSliceInstantiations;

        RootSliceAsset m_rootAsset;

        // The id of the entity context that created this EntityOwnershipService.
        EntityContextId m_entityContextId;

        //! Monotic tickets for slice instantiation requests.
        AZ::u64 m_nextSliceTicketId;

        //! When a slice is instantiating, the associated asset ID is cached here.
        AZ::Data::AssetId m_instantiatingAssetId;

        AZ::SerializeContext* m_serializeContext;

        //! Stores map from entity Ids loaded from stream, to remapped entity Ids, if remapping was performed.
        AZ::SliceComponent::EntityIdToEntityIdMap m_loadedEntityIdMap;
    };
}
