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

namespace Terrain
{

    //! This bus provides retrieval of information from Terrain Surfaces.
    class TerrainAreaSurfaceRequests
        : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;

        // This bus will not lock during an EBus call. This lets us run multiple queries in parallel, but it also means
        // that anything that implements this EBus will need to ensure that queries can't be in the middle of running at the
        // same time as bus connects / disconnects.
        static const bool LocklessDispatch = true;
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
