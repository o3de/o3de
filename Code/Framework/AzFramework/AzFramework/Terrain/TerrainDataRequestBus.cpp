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

    void FloatRange::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FloatRange>()
                ->Version(1)
                ->Field("Min", &FloatRange::m_min)
                ->Field("Max", &FloatRange::m_max)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<FloatRange>()
                ->Property("min", BehaviorValueProperty(&FloatRange::m_min))
                ->Property("max", BehaviorValueProperty(&FloatRange::m_max))
                ;
        }

    }

    TerrainQueryRegion::TerrainQueryRegion(
        const AZ::Vector3& startPoint, size_t numPointsX, size_t numPointsY, const AZ::Vector2& stepSize)
        : m_startPoint(startPoint)
        , m_numPointsX(numPointsX)
        , m_numPointsY(numPointsY)
        , m_stepSize(stepSize)
    {
    }

    TerrainQueryRegion::TerrainQueryRegion(const AZ::Vector2& startPoint, size_t numPointsX, size_t numPointsY, const AZ::Vector2& stepSize)
        : m_startPoint(startPoint)
        , m_numPointsX(numPointsX)
        , m_numPointsY(numPointsY)
        , m_stepSize(stepSize)
    {
    }

    TerrainQueryRegion TerrainQueryRegion::CreateFromAabbAndStepSize(const AZ::Aabb& region, const AZ::Vector2& stepSize)
    {
        TerrainQueryRegion queryRegion;

        if (region.IsValid() && (stepSize.GetX() > 0.0f) && (stepSize.GetY() > 0.0f))
        {
            queryRegion.m_startPoint = region.GetMin();
            queryRegion.m_stepSize = stepSize;

            const AZ::Vector3 regionExtents = region.GetExtents();

            // We'll treat the region as min-inclusive, max-exclusive.
            // Ex: (1,1) - (6,6) with stepSize(1) will process 1, 2, 3, 4, 5 but not 6 in each direction.
            // If the region is smaller than stepSize, make sure we still process at least the start point ("min-inclusive").
            // However, when an extent is zero-size (i.e. min == max), we need to choose whether to follow the "min-inclusive" rule
            // and include the start/end point, or the "max-exclusive" rule and exclude the start/end point. We're choosing to go
            // with "max-exclusive", since it seems likely that a 0-length extent wasn't intended to include any points.
            queryRegion.m_numPointsX = aznumeric_cast<size_t>(AZStd::ceil(regionExtents.GetX() / stepSize.GetX()));
            queryRegion.m_numPointsY = aznumeric_cast<size_t>(AZStd::ceil(regionExtents.GetY() / stepSize.GetY()));
        }

        return queryRegion;
    }

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
        FloatRange::Reflect(context);

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
                ->Event("GetTerrainHeightBounds", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightBounds)
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
