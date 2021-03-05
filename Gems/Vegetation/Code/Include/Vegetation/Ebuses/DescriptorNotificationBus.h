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
