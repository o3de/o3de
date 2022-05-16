/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DetourNavigationComponent.h"

#include "RecastHelpers.h"

#include <DetourDebugDraw.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
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

    AZStd::vector<AZ::Vector3> DetourNavigationComponent::FindPathBetweenPositions(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: FindPathBetweenPositions");

        dtNavMeshQuery* navQuery = nullptr;
        RecastNavigationMeshRequestBus::EventResult(navQuery, m_navQueryEntityId, &RecastNavigationMeshRequests::GetNativeNavigationQuery);

        if (!navQuery)
        {
            return {};
        }

        AZStd::vector<AZ::Vector3> pathPoints;
        RecastVector3 startRecast{ fromWorldPosition }, endRecast{ targetWorldPosition };
        const float halfExtents[3] = { m_nearestDistance, m_nearestDistance, m_nearestDistance };

        dtPolyRef startPoly = 0, endPoly = 0;

        RecastVector3 nearestStartPoint, nearestEndPoint;

        const dtQueryFilter filter;

        // Find nearest points on the navigation mesh.
        dtStatus result = navQuery->findNearestPoly(startRecast.GetData(), halfExtents, &filter, &startPoly, nearestStartPoint.GetData());
        if (dtStatusFailed(result))
        {
            return {};
        }

        result = navQuery->findNearestPoly(endRecast.GetData(), halfExtents, &filter, &endPoly, nearestEndPoint.GetData());
        if (dtStatusFailed(result))
        {
            return {};
        }

        constexpr int maxPathLength = 100;
        
        dtPolyRef path[maxPathLength] = {};
        int pathLength = 0;

        // Find an approximate path first.
        result = navQuery->findPath(startPoly, endPoly, nearestStartPoint.GetData(), nearestEndPoint.GetData(), &filter, path, &pathLength, maxPathLength);
        if (dtStatusFailed(result))
        {
            return {};
        }
        
        RecastVector3 detailedPath[maxPathLength] = {};
        AZ::u8 detailedPathFlags[maxPathLength] = {};
        dtPolyRef detailedPolyPathRefs[maxPathLength] = {};
        int detailedPathCount = 0;

        // Then the detailed path.
        result = navQuery->findStraightPath(startRecast.GetData(), endRecast.GetData(), path, pathLength, detailedPath[0].GetData(), detailedPathFlags, detailedPolyPathRefs,
            &detailedPathCount, maxPathLength, DT_STRAIGHTPATH_ALL_CROSSINGS);
        if (dtStatusFailed(result))
        {
            return {};
        }

        for (int i = 0; i < detailedPathCount; ++i)
        {
            pathPoints.push_back(detailedPath[i].AsVector3());
        }

        return pathPoints;
    }

    void DetourNavigationComponent::Activate()
    {
        DetourNavigationRequestBus::Handler::BusConnect(GetEntityId());
    }

    void DetourNavigationComponent::Deactivate()
    {
        DetourNavigationRequestBus::Handler::BusDisconnect();
    }
} // namespace RecastNavigation
