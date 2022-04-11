/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/EBusSharedDispatchTraits.h>
#include <AzCore/Math/Aabb.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfacePointList.h>

namespace SurfaceData
{
    /**
     * The EBus is used to request information about a surface.
     * This bus uses shared dispatches, which means that all requests on the bus can run in parallel, but will NOT run in parallel
     * with bus connections / disconnections.
     */
    class SurfaceDataModifierRequests : public AZ::EBusSharedDispatchTraits<SurfaceDataModifierRequests>
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::u32 BusIdType;
        ////////////////////////////////////////////////////////////////////////

        virtual void ModifySurfacePoints(
            AZStd::span<const AZ::Vector3> positions,
            AZStd::span<const AZ::EntityId> creatorEntityIds,
            AZStd::span<SurfaceData::SurfaceTagWeights> weights) const = 0;
    };

    typedef AZ::EBus<SurfaceDataModifierRequests> SurfaceDataModifierRequestBus;
}
