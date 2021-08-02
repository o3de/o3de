/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Slice/SliceInstantiationBus.h>

namespace AzFramework
{
    using EntityContextId = AZ::Uuid;
    using RootSliceAsset = AZ::Data::Asset<AZ::SliceAsset>;

    class SliceEntityOwnershipServiceRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EntityContextId;

        /**
         * Gets the Asset ID of the currently instantiating slice.
         * If no slice is currently being instantiated, it returns an invalid ID
         * @return The Asset ID of the slice currently being instantiated.
         */
        virtual AZ::Data::AssetId CurrentlyInstantiatingSlice() = 0;

        /**
         * Initialize this entity ownership service with a newly loaded root slice.
         * 
         * @param rootEntity the rootEntity which has been loaded
         * @param remapIds if true, entity Ids will be remapped post-load
         * @param idRemapTable if remapIds is true, the provided table is filled with a map of original ids to new ids
         * @return whether reading root entity was successful or not
         */
        virtual bool HandleRootEntityReloadedFromStream(AZ::Entity* rootEntity, bool remapIds,
            AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable = nullptr) = 0;

        virtual AZ::SliceComponent* GetRootSlice() = 0;

        /**
         * Returns a mapping of stream-loaded entity IDs to remapped entity IDs,if remapping was performed.
         * If the stream was loaded without remapping enabled, the map will be empty.
         * @return A mapping of entity IDs loaded from a stream to remapped values.
         */
        virtual const AZ::SliceComponent::EntityIdToEntityIdMap& GetLoadedEntityIdMap() = 0;

        /**
         * Returns the remapped id of a stream-loaded EntityId if remapping was performed.
         * @return The remapped EntityId
         *
         */
        virtual AZ::EntityId FindLoadedEntityIdMapping(const AZ::EntityId& staticId) const = 0;

        /**
         * Instantiate a slice asset in the entity ownership service. Listen for the OnSliceInstantiated() / OnSliceInstantiationFailed()
         * events for details about the resulting entities.
         * @param asset slice asset to instantiate.
         * @param customIdMapper optional Id map callback to allow caller to customize entity Id generation.
         * @param assetLoadFilterCB optional asset load filter callback. This is only necessary when heavily customizing asset loading,
         *        as it can allow deferral of dependent asset loading.
         * @param return slice instantiation ticket.
         */
        virtual SliceInstantiationTicket InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper = nullptr,
            const AZ::Data::AssetFilterCB& assetLoadFilter = nullptr) = 0;

        /**
         * Clones an existing slice instance in the entity ownership service. New instance is immediately returned.
         * This function doesn't automatically add new instance to the entity ownership service. Callers are responsible for that.
         * @param sourceInstance The source instance to be cloned
         * @param sourceToCloneEntityIdMap [out] The map between source entity ids and clone entity ids
         * @return new slice address. A null slice address will be returned if cloning fails (.first==nullptr, .second==nullptr).
         */
        virtual AZ::SliceComponent::SliceInstanceAddress CloneSliceInstance(AZ::SliceComponent::SliceInstanceAddress sourceInstance,
            AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap) = 0;

        /**
         * Cancels the asynchronous instantiation of a slice.
         * @param SliceInstantiationTicket The ticket identifies the asynchronous slice instantiation request.
         */
        virtual void CancelSliceInstantiation(const SliceInstantiationTicket& ticket) = 0;

        /**
         * Generates a ticket that can be used for tracking asynchronous slice instantiations.
         * @return SliceInstantiationTicket
         */
        virtual SliceInstantiationTicket GenerateSliceInstantiationTicket() = 0;

        /**
         * Enables the root slice to be a dynamic slice.
         * 
         * \param isDynamic
         */
        virtual void SetIsDynamic(bool isDynamic) = 0;

        virtual const RootSliceAsset& GetRootAsset() const = 0;
    };

    using SliceEntityOwnershipServiceRequestBus = AZ::EBus<SliceEntityOwnershipServiceRequests>;
}
