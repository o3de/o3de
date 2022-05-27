/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Components/DetourNavigationComponent.h>
#include <Misc/RecastHelpers.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    DetourNavigationComponent::DetourNavigationComponent(AZ::EntityId navQueryEntityId, float nearestDistance)
        : m_navQueryEntityId(navQueryEntityId), m_nearestDistance(nearestDistance)
    {
    }

    void DetourNavigationComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DetourNavigationComponent, AZ::Component>()
                ->Field("Navigation Query Entity", &DetourNavigationComponent::m_navQueryEntityId)
                ->Field("Nearest Distance", &DetourNavigationComponent::m_nearestDistance)
                ->Version(1);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<DetourNavigationRequestBus>("DetourNavigationRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Category, "Recast Navigation")
                ->Event("Find Path Between Entities", &DetourNavigationRequests::FindPathBetweenEntities)
                ->Event("Find Path Between Positions", &DetourNavigationRequests::FindPathBetweenPositions)
                ;

            behaviorContext->Class<DetourNavigationComponent>()->RequestBus("DetourNavigationRequestBus");
        }
    }

    AZStd::vector<AZ::Vector3> DetourNavigationComponent::FindPathBetweenEntities(AZ::EntityId fromEntity, AZ::EntityId toEntity)
    {
        if (fromEntity.IsValid() && toEntity.IsValid())
        {
            AZ::Vector3 start = AZ::Vector3::CreateZero(), end = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(start, fromEntity, &AZ::TransformBus::Events::GetWorldTranslation);
            AZ::TransformBus::EventResult(end, toEntity, &AZ::TransformBus::Events::GetWorldTranslation);

            return FindPathBetweenPositions(start, end);
        }

        return {};
    }

    AZStd::vector<AZ::Vector3> DetourNavigationComponent::FindPathBetweenPositions(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& toWorldPosition)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: FindPathBetweenPositions");

        AZStd::shared_ptr<NavMeshQuery> nav;
        RecastNavigationMeshRequestBus::EventResult(nav, m_navQueryEntityId, &RecastNavigationMeshRequests::GetNavigationObject);

        if (!nav)
        {
            return {};
        }

        AZStd::vector<AZ::Vector3> pathPoints;
        RecastVector3 startRecast{ fromWorldPosition }, endRecast{ toWorldPosition };
        const float halfExtents[3] = { m_nearestDistance, m_nearestDistance, m_nearestDistance };

        dtPolyRef startPoly = 0, endPoly = 0;

        RecastVector3 nearestStartPoint, nearestEndPoint;

        const dtQueryFilter filter;

        // Find nearest points on the navigation mesh given the positions provided.
        // We are allowing some flexibility where looking for a point just a bit outside of the navigation mesh would still work.
        dtStatus result = nav->m_query->findNearestPoly(startRecast.GetData(), halfExtents, &filter, &startPoly, nearestStartPoint.GetData());
        if (dtStatusFailed(result) || startPoly == 0)
        {
            return {};
        }

        result = nav->m_query->findNearestPoly(endRecast.GetData(), halfExtents, &filter, &endPoly, nearestEndPoint.GetData());
        if (dtStatusFailed(result) || endPoly == 0)
        {
            return {};
        }

        // Some reasonable amount of waypoints along the path. Recast isn't made to calculate very long paths.
        constexpr int maxPathLength = 100;

        dtPolyRef path[maxPathLength] = {};
        int pathLength = 0;

        // Find an approximate path first. In Recast, an approximate path is a collection of polygons, where a polygon covers an area.
        result = nav->m_query->findPath(startPoly, endPoly, nearestStartPoint.GetData(), nearestEndPoint.GetData(), &filter, path, &pathLength, maxPathLength);
        if (dtStatusFailed(result))
        {
            return {};
        }

        RecastVector3 detailedPath[maxPathLength] = {};
        AZ::u8 detailedPathFlags[maxPathLength] = {};
        dtPolyRef detailedPolyPathRefs[maxPathLength] = {};
        int detailedPathCount = 0;

        // Then the detailed path. This gives us actual specific waypoints along the path over the polygons found earlier.
        result = nav->m_query->findStraightPath(startRecast.GetData(), endRecast.GetData(), path, pathLength, detailedPath[0].GetData(), detailedPathFlags, detailedPolyPathRefs,
            &detailedPathCount, maxPathLength, DT_STRAIGHTPATH_ALL_CROSSINGS);
        if (dtStatusFailed(result))
        {
            return {};
        }

        // Note: Recast uses +Y, O3DE used +Z as up vectors.
        for (int i = 0; i < detailedPathCount; ++i)
        {
            pathPoints.push_back(detailedPath[i].AsVector3());
        }

        return pathPoints;
    }

    void DetourNavigationComponent::Activate()
    {
        if (!m_navQueryEntityId.IsValid())
        {
            // Default to looking for the navigation mesh component on the same entity if one is not specified.
            m_navQueryEntityId = GetEntityId();
        }

        DetourNavigationRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DetourNavigationComponent::Deactivate()
    {
        DetourNavigationRequestBus::Handler::BusDisconnect();
    }
} // namespace RecastNavigation
