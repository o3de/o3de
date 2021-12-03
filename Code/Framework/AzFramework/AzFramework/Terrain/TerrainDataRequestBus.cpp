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
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Event("GetHeight", &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetHeight)
                ->Event("GetHeightFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetHeightFromFloats)
                ->Event("GetHeightFromVector2", &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetHeightFromVector2)
                ->Event("GetNormal", &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetNormal)
                ->Event("GetMaxSurfaceWeight", &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetMaxSurfaceWeight)
                ->Event(
                    "GetMaxSurfaceWeightFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetMaxSurfaceWeightFromVector2)
                ->Event("GetSurfaceWeights", &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetSurfaceWeights)
                ->Event(
                    "GetSurfaceWeightsFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetSurfaceWeightsFromVector2)
                ->Event("GetIsHole", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHole)
                ->Event("GetIsHoleFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHoleFromFloats)
                ->Event("GetSurfacePoint", &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetSurfacePoint)
                ->Event(
                    "GetSurfacePointFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetSurfacePointFromVector2)
                ->Event("GetTerrainAabb", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb)
                ->Event(
                    "GetTerrainHeightQueryResolution",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightQueryResolution)
                ;
        }

    }
} // namespace AzFramework::Terrain
