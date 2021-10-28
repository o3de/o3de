/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

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
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainAreaSurfaceRequests() = default;

        //! Get the surfaces and weights from a gradient at a given position.
        virtual void GetSurfaceWeights(const AZ::Vector3& inPosition, AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights) const = 0;
    };

    using TerrainAreaSurfaceRequestBus = AZ::EBus<TerrainAreaSurfaceRequests>;
} // namespace Terrain
