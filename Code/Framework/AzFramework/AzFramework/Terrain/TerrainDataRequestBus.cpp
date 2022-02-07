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
                ->Event("GetIsHoleFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHoleFromFloats)
                ->Event("GetSurfacePoint", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetSurfacePoint)
                ->Event("GetSurfacePointFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetSurfacePointFromVector2)
                ->Event("GetTerrainAabb", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb)
                ->Event("GetTerrainHeightQueryResolution",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightQueryResolution)
                ;
        }

    }
} // namespace AzFramework::Terrain
