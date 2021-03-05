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

#include "TerrainDataRequestBus.h"

namespace AzFramework
{
    namespace SurfaceData
    {
        void SurfaceTagWeight::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SurfaceTagWeight>()
                    ->Field("m_surfaceType", &SurfaceTagWeight::m_surfaceType)
                    ->Field("m_weight", &SurfaceTagWeight::m_weight)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<SurfaceTagWeight>("SurfaceTagWeight")
                    ->Property("m_surfaceType", BehaviorValueProperty(&SurfaceTagWeight::m_surfaceType))
                    ->Property("m_weight", BehaviorValueProperty(&SurfaceTagWeight::m_weight))
                    ;
            }
        }
    } //namespace SurfaceData

    namespace Terrain
    {
        void TerrainDataRequests::Reflect(AZ::ReflectContext* context)
        {
            AzFramework::SurfaceData::SurfaceTagWeight::Reflect(context);

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<AzFramework::Terrain::TerrainDataRequestBus>("TerrainDataRequestBus")
                    ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                    ->Event("GetHeight", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetHeight)
                    ->Event("GetHeightFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetHeightFromFloats)
                    ->Event("GetMaxSurfaceWeight", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetMaxSurfaceWeight)
                    ->Event("GetMaxSurfaceWeightFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetMaxSurfaceWeightFromFloats)
                    ->Event("GetIsHoleFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHoleFromFloats)
                    ->Event("GetNormal", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetNormal)
                    ->Event("GetNormalFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetNormalFromFloats)
                    ->Event("GetTerrainAabb", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb)
                    ->Event("GetTerrainGridResolution", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainGridResolution)
                    ;

            }

        }

    } //namespace Terrain
} // namespace AzFramework
