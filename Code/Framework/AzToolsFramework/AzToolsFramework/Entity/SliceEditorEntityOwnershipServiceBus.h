/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzFramework/Slice/SliceInstantiationTicket.h>

namespace AZ
{
    class Transform;
}

namespace AzToolsFramework
{
    using EntityIdList = AZStd::vector<AZ::EntityId>;
    using EntityList = AZStd::vector<AZ::Entity*>;

    /**
     * Indicates how an entity was removed from its slice instance, so the said entity can be restored properly.
     */
    enum class SliceEntityRestoreType
    {
        Deleted,
        Detached,
        Added
    };

    class SliceEditorEntityOwnershipServiceRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AzFramework::SliceInstantiationTicket InstantiateEditorSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset,
            const AZ::Transform& worldTransform) = 0;

        /**
         * Clones an existing slice instance in the editor context. New instance is immediately returned.
         * This function doesn't automatically add the new slice instance to any entity context, callers are responsible for that.
         * 
         * \param sourceInstance The source instance to be cloned
         * \param sourceToCloneEntityIdMap The map between source entity ids and clone entity ids
         * \return The Address of new slice instance. A null address will be returned if the source instance address is invalid.
         */
        virtual AZ::SliceComponent::SliceInstanceAddress CloneEditorSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance,
            AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) = 0;

         /**
         * Clone an slice-instance that comes from a sub-slice, and add the clone to the root slice.
         * 
         * \param sourceSliceInstanceAddress The address of the slice instance that contains the sub-slice instance.
         * \param sourceSubSliceInstanceAncestry The ancestry in order from sourceSubSlice to sourceSlice
         * \param sourceSubSliceInstanceAddress The address of the sub-slice instance to be cloned.
         * \param out_sourceToCloneEntityIdMap If valid address provided, the internal source to clone entity ID map will be returned
          * \return The Address of new slice instance. A null address will be returned if the source instance address is invalid.
          */
        virtual AZ::SliceComponent::SliceInstanceAddress CloneSubSliceInstance(const AZ::SliceComponent::SliceInstanceAddress& sourceSliceInstanceAddress,
            const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sourceSubSliceInstanceAncestry,
            const AZ::SliceComponent::SliceInstanceAddress& sourceSubSliceInstanceAddress,
            AZ::SliceComponent::EntityIdToEntityIdMap* out_sourceToCloneEntityIdMap) = 0;

        /**
         * Moves existing entities in the EditorEntityOwnershipService into a new SliceInstance based off of the provided SliceAsset.
         *
         * \param sliceAsset Asset of the slice that the entities will be promoted into.
         * \param liveToAssetMap A mapping of the EntityIDs found in the provided SliceAsset and existing "live"
         *        EntityIDs found in the EditorEntityOwnershipService.
         * \return Can return an empty invalid SliceInstanceAddress if an error occurs during the process.
         */
        virtual AZ::SliceComponent::SliceInstanceAddress PromoteEditorEntitiesIntoSlice(const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
            const AZ::SliceComponent::EntityIdToEntityIdMap& liveToAssetMap) = 0;

        /**
         * Detaches entities from their current slice instance and adds them to root slice as loose entities.
         * 
         * \param entities Entities to detach.
         */
        virtual void DetachSliceEntities(const EntityIdList& entities) = 0;

        /**
         * Detaches all entities from input instances and adds them to the root slice as loose entities.
         * 
         * \param instances SliceInstances to detach
         */
        virtual void DetachSliceInstances(const AZ::SliceComponent::SliceInstanceAddressSet& instances) = 0;

        /**
         * Detaches the supplied subslices from their owning slice instance.
         * 
         * \param subsliceRootList Mapping of subslice instance address to entityIdMapping
         */
        virtual void DetachSubsliceInstances(const AZ::SliceComponent::SliceInstanceEntityIdRemapList& subsliceRootList) = 0;

        /**
         * Restores an entity back to a slice instance for undo/redo *only*. A valid \ref EntityRestoreInfo must be provided, 
         * and is only extracted directly via \ref SliceReference::GetEntityRestoreInfo().
         * 
         * \param entity The entity to restore to the slice
         * \param info The EntityRestoreInfo reference to be used to restore the entity
         * \param restoreType The SliceEntityRestoreType
         */
        virtual void RestoreSliceEntity(AZ::Entity* entity, const AZ::SliceComponent::EntityRestoreInfo& info,
            SliceEntityRestoreType restoreType) = 0;

        /// Resets any slice data overrides for the specified entities
        virtual void ResetEntitiesToSliceDefaults(EntityIdList entities) = 0;

        /// Retrieves the root slice for the editor entity ownership service.
        virtual AZ::SliceComponent* GetEditorRootSlice() = 0;
    };

    using SliceEditorEntityOwnershipServiceRequestBus = AZ::EBus<SliceEditorEntityOwnershipServiceRequests>;

    class SliceEditorEntityOwnershipServiceNotifications
        : public AZ::EBusTraits
    {
    public:
        /// Fired when a slice has been successfully instantiated.
        virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/,
            const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

        /// Fired when a slice has failed to instantiate.
        virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/,
            const AzFramework::SliceInstantiationTicket& /*ticket*/) {}

        // When a slice is created, the editor entities are moved into the first instance of the slice, this notification
        // sends a list of the promoted entities
        virtual void OnEditorEntitiesPromotedToSlicedEntities(const EntityIdList& /*promotedEntities*/) {}

        /// Fired when an group of entities has slice ownership change.
        /// This should only be fired if all of the entities now belong to the same slice or all now belong to no slice
        virtual void OnEditorEntitiesSliceOwnershipChanged(const EntityIdList& /*entityIdList*/) {}

        //! Fired before the Editor entity ownership service exports the root level slice to the game stream
        virtual void OnSaveStreamForGameBegin(AZ::IO::GenericStream& /*gameStream*/, AZ::DataStream::StreamType /*streamType*/, AZStd::vector<AZStd::unique_ptr<AZ::Entity>>& /*levelEntities*/) {}

        //! Fired after the Editor entity ownership service exports the root level slice to the game stream
        virtual void OnSaveStreamForGameSuccess(AZ::IO::GenericStream& /*gameStream*/) {}

        //! Fired after the Editor entity ownership service fails to export the root level slice to the game stream
        virtual void OnSaveStreamForGameFailure(AZStd::string_view /*failureString*/) {}
    };

    using SliceEditorEntityOwnershipServiceNotificationBus = AZ::EBus<SliceEditorEntityOwnershipServiceNotifications>;
}
