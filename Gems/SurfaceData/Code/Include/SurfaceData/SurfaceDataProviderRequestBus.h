/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/span.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfacePointList.h>

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

        //! Get all of the surface points that this provider has at the given input position.
        //! @param inPosition - The input position to query. Only XY are guaranteed to be valid, Z should be ignored.
        //! @param surfacePointList - The input/output SurfacePointList to add any generated surface points to.
        virtual void GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const = 0;

        //! Get all of the surface points that this provider has at the given input positions.
        //! @param inPositions - The input positions to query. Only XY are guaranteed to be valid, Z should be ignored.
        //! @param surfacePointList - The input/output SurfacePointList to add any generated surface points to.
        virtual void GetSurfacePointsFromList(AZStd::span<const AZ::Vector3> inPositions, SurfacePointList& surfacePointList) const
        {
            for (auto& inPosition : inPositions)
            {
                GetSurfacePoints(inPosition, surfacePointList);
            }
        }
    };

    typedef AZ::EBus<SurfaceDataProviderRequests> SurfaceDataProviderRequestBus;
}
