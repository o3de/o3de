/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TerrainDataRequestBus.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AzFramework::Terrain
{
    void TerrainDataRequests::Reflect(AZ::ReflectContext* context)
    {
        // These APIs are overloaded, so we need to define aliases for the version of the APIs that we want to use.
        // Otherwise the BehaviorContext definition below won't compile.
        using GetSurfacePointFuncPtr = SurfaceData::SurfacePoint (TerrainDataRequests::*)(const AZ::Vector3&, Sampler) const;
        using GetSurfacePointFromVector2FuncPtr = SurfaceData::SurfacePoint (TerrainDataRequests::*)(const AZ::Vector2&, Sampler) const;

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AzFramework::Terrain::TerrainDataRequestBus>("TerrainDataRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Event("GetHeight", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetHeight)
                ->Event("GetNormal", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetNormal)
                ->Event("GetMaxSurfaceWeight", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetMaxSurfaceWeight)
                ->Event("GetMaxSurfaceWeightFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetMaxSurfaceWeightFromVector2)
                ->Event("GetSurfaceWeights", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetSurfaceWeights)
                ->Event("GetSurfaceWeightsFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetSurfaceWeightsFromVector2)
                ->Event("GetIsHole", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHole)
                ->Event("GetIsHoleFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHoleFromFloats)
                ->Event("GetSurfacePoint",
                    static_cast<GetSurfacePointFuncPtr>(&AzFramework::Terrain::TerrainDataRequestBus::Events::GetSurfacePoint))
                ->Event("GetSurfacePointFromVector2",
                    static_cast<GetSurfacePointFromVector2FuncPtr>(&AzFramework::Terrain::TerrainDataRequestBus::Events::GetSurfacePointFromVector2))
                ->Event("GetTerrainAabb", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb)
                ->Event("GetTerrainHeightQueryResolution",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightQueryResolution)
                ;
        }

    }
} // namespace AzFramework::Terrain
