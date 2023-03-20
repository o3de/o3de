/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/containers/span.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <AzCore/EBus/EBusSharedDispatchTraits.h>

namespace Terrain
{

    //! This bus provides retrieval of information from Terrain Surfaces.
    //! This bus uses shared dispatches, which means that all requests on the bus can run in parallel,
    //! but will NOT run in parallel with bus connections / disconnections.
    class TerrainAreaSurfaceRequests : public AZ::EBusSharedDispatchTraits<TerrainAreaSurfaceRequests>
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainAreaSurfaceRequests() = default;

        //! Get the surfaces and weights from a gradient at a given position.
        virtual void GetSurfaceWeights(const AZ::Vector3& inPosition, AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights) const = 0;

        //! Get the surfaces and weights from a gradient at a given list of positions.
        virtual void GetSurfaceWeightsFromList(
            AZStd::span<const AZ::Vector3> inPositionList,
            AZStd::span<AzFramework::SurfaceData::SurfaceTagWeightList> outSurfaceWeightsList) const = 0;
    };

    using TerrainAreaSurfaceRequestBus = AZ::EBus<TerrainAreaSurfaceRequests>;
} // namespace Terrain
