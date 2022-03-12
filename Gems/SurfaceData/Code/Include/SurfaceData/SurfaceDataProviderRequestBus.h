/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace SurfaceData
{
    /**
    * the EBus is used to request information about a surface
    */
    class SurfaceDataProviderRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::u32 BusIdType;
        ////////////////////////////////////////////////////////////////////////

        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        virtual void GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const = 0;
    };

    typedef AZ::EBus<SurfaceDataProviderRequests> SurfaceDataProviderRequestBus;
}
