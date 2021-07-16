/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzFramework/Slice/SliceInstantiationTicket.h>

namespace AzFramework
{

    class SliceGameEntityOwnershipServiceRequests
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        /**
         * Instantiates a dynamic slice asynchronously.
         * @param sliceAsset A reference to the slice asset data.
         * @param worldTransform A reference to the world transform to apply to the slice.
         * @param customIdMapper An ID mapping function that is used when instantiating the slice.
         * @return A ticket that identifies the slice instantiation request. Callers can immediately
         * subscribe to the AzFramework::SliceInstantiationResultBus for this ticket to receive results
         * for this request.
         */
        virtual SliceInstantiationTicket InstantiateDynamicSlice(const AZ::Data::Asset<AZ::Data::AssetData>& /*sliceAsset*/,
            const AZ::Transform& /*worldTransform*/, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& /*customIdMapper*/) = 0;

        /**
         * Cancels the asynchronous instantiation of a dynamic slice.
         * This call has no effect if the slice has already finished instantiation.
         * @param ticket The ticket that identifies the slice instantiation request.
         */
        virtual void CancelDynamicSliceInstantiation(const SliceInstantiationTicket& /*ticket*/) = 0;

        /**
         * Destroys an entire dynamic slice instance given the ID of any entity within the slice.
         * @param id The ID of the entity whose dynamic slice instance you want to destroy.
         * @return True if the dynamic slice instance was successfully destroyed. Otherwise, false.
         */
        virtual bool DestroyDynamicSliceByEntity(const AZ::EntityId& /*id*/) = 0;
    };

    using SliceGameEntityOwnershipServiceRequestBus = AZ::EBus<SliceGameEntityOwnershipServiceRequests>;

    class SliceGameEntityOwnershipServiceNotifications
        : public AZ::EBusTraits
    {
    public:

        /**
         * Signals that a slice was instantiated successfully.
         * @param sliceAssetId The asset ID of the slice to instantiate.
         * @param instance The slice instance.
         * @param ticket A ticket that identifies the slice instantiation request.
         */
        virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*instance*/, const SliceInstantiationTicket& /*ticket*/) {}

        /**
         * Signals that a slice asset could not be instantiated.
         * @param sliceAssetId The asset ID of the slice that failed to instantiate.
         * @param ticket A ticket that identifies the slice instantiation request.
         */
        virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/, const SliceInstantiationTicket& /*ticket*/) {}
    };

    using SliceGameEntityOwnershipServiceNotificationBus = AZ::EBus<SliceGameEntityOwnershipServiceNotifications>;
}
