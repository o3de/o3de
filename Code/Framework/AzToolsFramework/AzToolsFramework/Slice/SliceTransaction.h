
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

namespace AZ
{
    class Entity;
    class SerializeContext;
}

namespace AzToolsFramework
{
    namespace SliceUtilities
    {
        /**
         * Utility class for performing transactional operations on slices, such as creating or pushing changes to slices.
         * Use SliceTransaction::BeginNewSlice or SliceTransaction::BeginSlicePush to create a new transaction.
         * See the above methods' API documentation for more information about populating and committing transactions.
         *
         * A single transaction should not be interacted with simultaneously from different threads.
         * However, it is safe to pass a transaction pointer across threads, to jobs, etc.
         */
        class SliceTransaction
        {
        public:

            AZ_CLASS_ALLOCATOR(SliceTransaction, AZ::SystemAllocator);

            typedef AZ::Outcome<void, AZStd::string> Result;
            typedef AZStd::intrusive_ptr<SliceTransaction> TransactionPtr;
            typedef AZ::Data::Asset<AZ::SliceAsset> SliceAssetPtr;
            typedef AZStd::function<Result(TransactionPtr, const char*, SliceAssetPtr&)> PreSaveCallback;
            typedef AZStd::function<void(TransactionPtr, const char*, const SliceAssetPtr&)> PostSaveCallback;

            friend struct AZStd::IntrusivePtrCountPolicy<SliceTransaction>;

            /**
             * Flags passed to BeginNewSlice().
             */
            enum SliceCreationFlags
            {
                CreateAsDynamic                 = (1<<0),
            };

            /**
             * Flags passed to BeginSlicePush().
             */
            enum SlicePushFlags
            {
            };

            /**
             * Flags passed to AddEntity().
             */
            enum SliceAddEntityFlags
            {
                DiscardSliceAncestry            = (1<<0),       ///< Adds the entity as a loose entity, detaching it from any existing slice hierarchy, meaning it will no longer inherit changes to any slice instances it was part of
            };

            /**
             * Flags passed to Commit().
             */
            enum SliceCommitFlags
            {
                DisableUndoCapture              = (1<<0),       ///< Disables undo batches from being created within the transaction
            };

            ~SliceTransaction();

            /**
             * Begin a transaction for creating a new slice.
             * Entities and nested slice instances can be added via AddEntity() and AddSliceInstance().
             * Use AddEntity() to add new entities to the slice.
             * Use AddSliceInstance() to add new nested slice instances.
             * \param name - Optional internal naming for slice. Will use "Slice" if none is provided.
             * \param serializeContext - Optional serialize context instance. Global serialize context will be used if none is provided.
             * \param sliceCreationFlags - See \ref SliceCreationFlags
             * \return Always returns a valid pointer to the new transaction.
             */
            static TransactionPtr BeginNewSlice(const char* name, AZ::SerializeContext* serializeContext = nullptr, AZ::u32 sliceCreationFlags = 0);

            /**
            * Begin a transaction for overwriting a slice with another slice component
            * \param asset - Pointer to Slice asset being overwritten
            * \param overwriteComponent - Slice component containing overwrite data
            * \param serializeContext - Optional serialize context instance. Global serialize context will be used if none is provided.
            * \return pointer to the new transaction, is null if the specified asset is invalid.
            */
            static TransactionPtr BeginSliceOverwrite(const SliceAssetPtr& asset, const AZ::SliceComponent& overwriteComponent, AZ::SerializeContext* serializeContext = nullptr);

            /**
             * Begin a transaction for pushing changes to an existing slice asset.
             * Use AddEntity() to add new entities to the slice.
             * Use AddSliceInstance() to add new nested slice instances.
             * Use UpdateEntity() to update whole existing entities.
             * Use UpdateEntityField() to update a single field on an existing entity.
             * \param asset - Pointer to slice asset to which changes are being pushed.
             * \param serializeContext - Optional serialize context instance. Global serialize context will be used if none is provided.
             * \param slicePushFlags - See \ref SlicePushFlags
             * \return pointer to the new transaction, is null if the specified asset is invalid.
             */
            static TransactionPtr BeginSlicePush(const SliceAssetPtr& asset, AZ::SerializeContext* serializeContext = nullptr, AZ::u32 slicePushFlags = 0);

            /**
             * For push transactions only: Adds a live entity to the transaction. Entity's data will be pushed to its ancestor in the slice.
             * \param entity - live entity to update.
             * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
             */
            Result UpdateEntity(AZ::Entity* entity);

            /**
            * For push transactions only: Adds a live entity to the transaction. Entity's data will be pushed to its ancestor in the slice.
            * \param entity - live entity to update.
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result UpdateEntity(const AZ::EntityId& entityId);

            /**
            * For push transactions only: Adds a live entity to the transaction, but with a specific field address. Field data will be pushed to its ancestor in the slice.
            * \param entity - live entity to update.
            * \param fieldNodeAddress - InstanceDataHierarchy address of the field to push to the entity in the slice.
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result UpdateEntityField(AZ::Entity* entity, const InstanceDataNode::Address& fieldNodeAddress);

            /**
            * For push transactions only: Adds a live entity to the transaction, but with a specific field address. Field data will be pushed to its ancestor in the slice.
            * \param entity - live entity to update.
            * \param fieldNodeAddress - InstanceDataHierarchy address of the field to push to the entity in the slice.
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result UpdateEntityField(const AZ::EntityId& entityId, const InstanceDataNode::Address& fieldNodeAddress);

            /**
            * For new slice or push transactions. Adds a new entity to the target slice, keeping slice ancestry by default if it is part
            * of a slice. Use SliceAddEntityFlags::DiscardSliceAncestry to add as a loose entity.
            * \param entity - live entity to add.
            * \param addEntityFlags - \ref SliceAddEntityFlags
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result AddEntity(const AZ::Entity* entity, AZ::u32 addEntityFlags = 0);

            /**
            * For new slice or push transactions. Adds a new entity to the target slice, keeping slice ancestry by default if it is part
            * of a slice. Use SliceAddEntityFlags::DiscardSliceAncestry to add as a loose entity.
            * \param entity - live entity to add.
            * \param addEntityFlags - \ref SliceAddEntityFlags
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result AddEntity(AZ::EntityId entityId, AZ::u32 addEntityFlags = 0);

            /**
             * For push transactions only: Removes an existing entity from the slice.
             * \param entity - live entity whose ancestor should be removed from the slice, or entity directly in target slice to remove.
             * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
             */
            Result RemoveEntity(AZ::Entity* entity);

            /**
            * For push transactions only: Removes an existing entity from the slice.
            * \param entity - live entity whose ancestor should be removed from the slice, or entity directly in target slice to remove.
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result RemoveEntity(AZ::EntityId entityId);

            /**
            * For new slice or push transactions. Adds a live slice instance to be nested in the target slice.
            * \param sliceAddress - pointer to live slice instance.
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result AddSliceInstance(const AZ::SliceComponent::SliceInstanceAddress& sliceAddress);

            /**
            * Completes and commits the transaction to disk at the specified location.
            * \param fullPath - full path to which the asset will be written.
            * \param preSaveCallback - user providable callback to pre-process the asset just prior to writing to disk.
            * \param postSaveCallback - user providable callback invoked just after the asset is written to disk.
            * \param sliceCommitFlags - \ref SliceCommitFlags
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result Commit(const char* fullPath, PreSaveCallback preSaveCallback, PostSaveCallback postSaveCallback, AZ::u32 sliceCommitFlags = 0);

            /**
            * Completes and commits the transaction to disk at the specified location.
            * \param targetAssetId - Asset Id to be committed.
            * \param preSaveCallback - user providable callback to pre-process the asset just prior to writing to disk.
            * \param postSaveCallback - user providable callback invoked just after the asset is written to disk.
            * \param sliceCommitFlags - \ref SliceCommitFlags
            * \return Result, AZ::Success if successful, otherwise AZ::Failure. Use GetError() to retrieve string describing error.
            */
            Result Commit(const AZ::Data::AssetId& targetAssetId, PreSaveCallback preSaveCallback, PostSaveCallback postSaveCallback, AZ::u32 sliceCommitFlags = SliceCommitFlags::DisableUndoCapture);

            /**
             * Retrieves an EntityId->EntityId map from the live entities that were added to the slice individually or as instances,
             * to the Ids of the corresponding ancestor within the target slice.
             * This can be useful if after creating a slice, you'd like a full mapping from the live entities used to create the slice
             * to their respective entities in the asset.
             */
            const AZ::SliceComponent::EntityIdToEntityIdMap& GetLiveToAssetEntityIdMap() const;

            bool AddLiveToAssetEntityIdMapping(const AZStd::pair<AZ::EntityId, AZ::EntityId>& mapping);

            const AZ::SliceComponent::EntityIdToEntityIdMap& GetAddedEntityIdRemaps() const;

            /**
             * Retrieves a pointer to the target asset. This is a modified clone of the original target asset that shares the correct asset id.
             */
            const SliceAssetPtr& GetTargetAsset() const { return m_targetAsset; }

            /**
            * Retrieves a pointer to the original target asset.
            * If this is a new slice operation it will be an empty asset as there was no original target.
            * If this is to update an existing slice it will represent the slice asset being updated.
            */
            const SliceAssetPtr& GetOriginalTargetAsset() const { return m_originalTargetAsset; }

        private:
    
            SliceTransaction(AZ::SerializeContext* serializeContext = nullptr);

            /// Clone asset in preparation for final write. PreSave operations will be applied to the clone.
            SliceAssetPtr CloneAssetForSave();

            /// Applies enabled pre-save behavior, and invokes user pre-save callback.
            Result PreSave(const char* fullPath, SliceAssetPtr& asset, PreSaveCallback preSaveCallback, AZ::u32 sliceCommitFlags);

            /// Locate an entity's corresponding ancestor in the transaction's target slice.
            /// If the ancestor is found, the corresponding Id entry is added to the provided idMap.
            AZ::EntityId FindTargetAncestorAndUpdateInstanceIdMap(AZ::EntityId id, AZ::SliceComponent::EntityIdToEntityIdMap& idMap, const AZ::SliceComponent::SliceInstanceAddress* ignoreSliceInstance = nullptr) const;

            /// Resets the transaction.
            void Reset();

            /// intrusive_ptr
            void add_ref();
            void release();

        private:

            SliceTransaction(const SliceTransaction&) = delete;
            SliceTransaction& operator=(const SliceTransaction&) = delete;

            enum class TransactionType
            {
                None,
                NewSlice,
                UpdateSlice,
                OverwriteSlice,
            };

            struct EntityToPush
            {
                EntityToPush(AZ::EntityId targetEntityId, AZ::EntityId sourceEntityId, const InstanceDataNode::Address& nodeAddress = InstanceDataNode::Address())
                    : m_targetEntityId(targetEntityId)
                    , m_sourceEntityId(sourceEntityId)
                    , m_fieldNodeAddress(nodeAddress) {}

                AZ::EntityId                m_targetEntityId;
                AZ::EntityId                m_sourceEntityId;
                InstanceDataNode::Address   m_fieldNodeAddress;
            };

            struct SliceInstanceToPush
            {
                SliceInstanceToPush()
                    : m_includeEntireInstance(false)
                    , m_instanceAddress(nullptr, nullptr)
                {}

                bool                                            m_includeEntireInstance;    ///< Whether to include all entities of the instance
                AZStd::unordered_set<AZ::EntityId>              m_entitiesToInclude;        ///< If m_includeEntireInstance == false, the entities we want to include
                AZ::SliceComponent::SliceInstanceAddress        m_instanceAddress;          ///< Source slice instance address
            };

            typedef AZStd::unordered_map<AZ::SliceComponent::SliceInstanceAddress, SliceInstanceToPush> SliceInstancesToPushMap;

            TransactionType                                             m_transactionType;
            AZ::SerializeContext*                                       m_serializeContext;
            SliceAssetPtr                                               m_originalTargetAsset;  ///< For Slice Pushes, the original in-memory asset passed to BeginSlicePush
            SliceAssetPtr                                               m_targetAsset;          ///< The asset in-memory that the transaction is making changes to (for creation, new one, for pushes, clone of assetToReplace)
            SliceInstancesToPushMap                                     m_addedSliceInstances;
            AZStd::vector<EntityToPush>                                 m_entitiesToPush;
            AZStd::vector<AZ::EntityId>                                 m_entitiesToRemove;
            AZ::SliceComponent::EntityIdToEntityIdMap                   m_liveToAssetIdMap;
            bool                                                        m_hasEntityAdds = false;///< Whether entities have been added as part of this transaction
            AZStd::unordered_map<AZ::EntityId, AZ::EntityId>            m_addedEntityIdRemaps;

            AZStd::atomic_int                                           m_refCount; // intrusive_ptr
        };

    } // namespace Slice
} // namespace AzToolsFramework
