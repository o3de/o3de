/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include<AzFramework/Slice/SliceInstantiationTicket.h>

namespace AzFramework
{
    /**
     * Interface for AzFramework::SliceInstantiationResultBus, which
     * enables you to receive results regarding your slice instantiation
     * requests.
     */
    class SliceInstantiationResults
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~SliceInstantiationResults() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusAddressPolicy to specify that the EBus
         * has multiple addresses. Components that request slice instantiation
         * receive the results of the request at the EBus address that is
         * associated with the request ID of the slice instantiation ticket.
         */
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        /**
         * Specifies that events are addressed by the request ID of the
         * slice instantiation ticket.
         */
        using BusIdType = SliceInstantiationTicket;

        //////////////////////////////////////////////////////////////////////////

        /**
         * Signals that a slice was successfully instantiated prior to
         * entity registration.
         * @param sliceAssetId A reference to the slice asset ID.
         * @param sliceAddress A reference to the slice instance address.
         */
        virtual void OnSlicePreInstantiate(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/) {}

        /**
         * Signals that a slice was successfully instantiated after
         * entity registration.
         * @param sliceAssetId A reference to the slice asset ID.
         * @param sliceAddress A reference to the slice instance address.
         */
        virtual void OnSliceInstantiated(const AZ::Data::AssetId& /*sliceAssetId*/, const AZ::SliceComponent::SliceInstanceAddress& /*sliceAddress*/) {}

        /**
         * Signals that a slice could not be instantiated.
         * @deprecated Please use OnSliceInstantiationFailedOrCanceled
         * @param sliceAssetId A reference to the slice asset ID.
         */
        virtual void OnSliceInstantiationFailed(const AZ::Data::AssetId& /*sliceAssetId*/) {}

        /**
         * Signals that a slice could not be instantiated.
         * @param sliceAssetId A reference to the slice asset ID.
         * @param canceled Set to true if the failure was due to cancellation.
         */
        virtual void OnSliceInstantiationFailedOrCanceled(const AZ::Data::AssetId& /*sliceAssetId*/, bool /*canceled*/) {}
    };

    /**
     * The EBus that notifies you about the results of your slice instantiation requests.
     * The events are defined in the AzFramework::SliceInstantiationResults class.
     */
    using SliceInstantiationResultBus = AZ::EBus<SliceInstantiationResults>;
}
