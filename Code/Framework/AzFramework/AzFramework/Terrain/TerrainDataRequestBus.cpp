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
    // Create a handler that can be accessed from Python scripts to receive terrain change notifications.
    class TerrainDataNotificationHandler final
        : public AzFramework::Terrain::TerrainDataNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            TerrainDataNotificationHandler,
            "{A83EF103-295A-4653-8279-F30FBF3F9037}",
            AZ::SystemAllocator,
            OnTerrainDataCreateBegin,
            OnTerrainDataCreateEnd,
            OnTerrainDataDestroyBegin,
            OnTerrainDataDestroyEnd,
            OnTerrainDataChanged);

        void OnTerrainDataCreateBegin() override
        {
            Call(FN_OnTerrainDataCreateBegin);
        }

        void OnTerrainDataCreateEnd() override
        {
            Call(FN_OnTerrainDataCreateEnd);
        }

        void OnTerrainDataDestroyBegin() override
        {
            Call(FN_OnTerrainDataDestroyBegin);
        }

        void OnTerrainDataDestroyEnd() override
        {
            Call(FN_OnTerrainDataDestroyEnd);
        }

        void OnTerrainDataChanged(
            const AZ::Aabb& dirtyRegion, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask dataChangedMask) override
        {
            Call(FN_OnTerrainDataChanged, dirtyRegion, dataChangedMask);
        }
    };

    void TerrainDataRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Register all the tuple types used below so that they marshal to/from python correctly.
            serializeContext->RegisterGenericType<AZStd::tuple<float, bool>>();
            serializeContext->RegisterGenericType<AZStd::tuple<AZ::Vector3, bool>>();
            serializeContext->RegisterGenericType<AZStd::tuple<SurfaceData::SurfaceTagWeight, bool>>();
            serializeContext->RegisterGenericType<AZStd::tuple<SurfaceData::SurfaceTagWeightList, bool>>();
            serializeContext->RegisterGenericType<AZStd::tuple<bool, AZ::Vector3, AZ::Vector3>>();
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AzFramework::Terrain::TerrainDataRequestBus>("TerrainDataRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Event(
                    "GetTerrainHeightQueryResolution",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightQueryResolution)
                ->Event(
                    "SetTerrainHeightQueryResolution",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::SetTerrainHeightQueryResolution)
                ->Event(
                    "GetTerrainSurfaceDataQueryResolution",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainSurfaceDataQueryResolution)
                ->Event(
                    "SetTerrainSurfaceDataQueryResolution",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::SetTerrainSurfaceDataQueryResolution)
                ->Event("GetTerrainAabb", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb)
                ->Event(
                    "GetHeight",
                    [](AzFramework::Terrain::TerrainDataRequests* handler, const AZ::Vector3& position,
                       Sampler sampler) -> AZStd::tuple<float, bool>
                    {
                        bool terrainExists = false;
                        float height = handler->GetHeight(position, sampler, &terrainExists);
                        return AZStd::make_tuple(height, terrainExists);
                    })
                ->Event("GetIsHole", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHole)
                ->Event(
                    "GetNormal",
                    [](AzFramework::Terrain::TerrainDataRequests* handler, const AZ::Vector3& position,
                       Sampler sampleFilter) -> AZStd::tuple<AZ::Vector3, bool>
                    {
                        bool terrainExists = false;
                        AZ::Vector3 normal = handler->GetNormal(position, sampleFilter, &terrainExists);
                        return AZStd::make_tuple(normal, terrainExists);
                    })
                ->Event(
                    "GetMaxSurfaceWeight",
                    [](AzFramework::Terrain::TerrainDataRequests* handler, const AZ::Vector3& position,
                       Sampler sampleFilter) -> AZStd::tuple<SurfaceData::SurfaceTagWeight, bool>
                    {
                        bool terrainExists = false;
                        SurfaceData::SurfaceTagWeight maxWeight = handler->GetMaxSurfaceWeight(position, sampleFilter, &terrainExists);
                        return AZStd::make_tuple(maxWeight, terrainExists);
                    })
                ->Event(
                    "GetSurfaceWeights",
                    [](AzFramework::Terrain::TerrainDataRequests* handler, const AZ::Vector3& inPosition,
                       Sampler sampleFilter) -> AZStd::tuple<SurfaceData::SurfaceTagWeightList, bool>  
                    {
                        bool terrainExists = false;
                        SurfaceData::SurfaceTagWeightList list;
                        handler->GetSurfaceWeights(inPosition, list, sampleFilter, &terrainExists);
                        return AZStd::make_tuple(list, terrainExists);
                    })
                ->Event(
                    "GetSurfacePoint",
                    [](AzFramework::Terrain::TerrainDataRequests* handler, const AZ::Vector3& inPosition,
                       Sampler samplerFilter) -> SurfaceData::SurfacePoint
                    {
                        SurfaceData::SurfacePoint result;
                        handler->GetSurfacePoint(inPosition, result, samplerFilter);
                        return result;
                    })
                ->Event(
                    "GetClosestIntersection",
                    [](AzFramework::Terrain::TerrainDataRequests* handler, const AZ::Vector3& startPosition,
                       const AZ::Vector3& endPosition) -> AZStd::tuple<bool, AZ::Vector3, AZ::Vector3>
                    {
                        RenderGeometry::RayRequest request;
                        request.m_startWorldPosition = startPosition;
                        request.m_endWorldPosition = endPosition;
                        RenderGeometry::RayResult result = handler->GetClosestIntersection(request);

                        return AZStd::make_tuple(result.m_distance < FLT_MAX, result.m_worldPosition, result.m_worldNormal);
                    })
                ;

            behaviorContext->EBus<AzFramework::Terrain::TerrainDataNotificationBus>("TerrainDataNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Handler<AzFramework::Terrain::TerrainDataNotificationHandler>()
            ;
        }
    }
} // namespace AzFramework::Terrain
