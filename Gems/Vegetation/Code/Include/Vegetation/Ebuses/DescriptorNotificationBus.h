/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//////////////////////////////////////////////////////////////////////////
//
// Ebus support for triggering necessary update of Entity/Object when
// associated characterInstances .chrparams file is hot loaded
//
//////////////////////////////////////////////////////////////////////////

#pragma once

#include <AzCore/EBus/EBus.h>

namespace Vegetation
{
    /**
    * listener for bounding changes coming from an character reload.
    */
    class DescriptorNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides

        // Send notifications per-descriptor
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef void* BusIdType;

        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        DescriptorNotifications() = default;
        virtual ~DescriptorNotifications() = default;

        /// Called whenever the descriptor assets have loaded
        virtual void OnDescriptorAssetsLoaded() {}

        /// Called whenever the descriptor assets have unloaded
        virtual void OnDescriptorAssetsUnloaded() {}

    };

    using DescriptorNotificationBus = AZ::EBus<DescriptorNotifications>;
} // Vegetation namespace
