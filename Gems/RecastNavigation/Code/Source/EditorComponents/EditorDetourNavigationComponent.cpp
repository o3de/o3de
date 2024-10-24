/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDetourNavigationComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <Components/DetourNavigationComponent.h>

#if defined(CARBONATED)
#include <AzCore/Component/TransformBus.h>

#include <RecastNavigation/RecastHelpers.h>
#include <RecastNavigation/RecastNavigationMeshBus.h>
#endif

namespace RecastNavigation
{
    void EditorDetourNavigationComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorDetourNavigationComponent, AZ::Component>()
                ->Field("Navigation Mesh", &EditorDetourNavigationComponent::m_navQueryEntityId)
                ->Field("Nearest Distance", &EditorDetourNavigationComponent::m_nearestDistance)
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorDetourNavigationComponent>("Detour Navigation Component",
                    "[Calculates paths within an associated navigation mesh.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDetourNavigationComponent::m_navQueryEntityId,
                        "Navigation Mesh", "Entity with Recast Navigation Mesh component")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDetourNavigationComponent::m_nearestDistance,
                        "Nearest Distance", "If FindPath APIs are given points that are outside the navigation mesh, then "
                        "look for the nearest point on the navigation mesh within this distance from the specified positions.")
                    ;
            }
        }
    }

    void EditorDetourNavigationComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DetourNavigationComponent"));
    }

    void EditorDetourNavigationComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DetourNavigationComponent"));
    }

    void EditorDetourNavigationComponent::Activate()
    {
        EditorComponentBase::Activate();
#if defined(CARBONATED)
        DetourNavigationRequestBus::Handler::BusConnect(GetEntityId());
#endif
    }

    void EditorDetourNavigationComponent::Deactivate()
    {
#if defined(CARBONATED)
        DetourNavigationRequestBus::Handler::BusDisconnect();
#endif
        EditorComponentBase::Deactivate();
    }

    void EditorDetourNavigationComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<DetourNavigationComponent>(m_navQueryEntityId, m_nearestDistance);
    }

#if defined(CARBONATED)
    void EditorDetourNavigationComponent::SetNavigationMeshEntity(AZ::EntityId navMeshEntity)
    {
        m_navQueryEntityId = navMeshEntity;
    }
    AZ::EntityId EditorDetourNavigationComponent::GetNavigationMeshEntity() const
    {
        return m_navQueryEntityId;
    }
    AZStd::vector<AZ::Vector3> EditorDetourNavigationComponent::FindPathBetweenEntities(
        AZ::EntityId fromEntity, AZ::EntityId toEntity, bool addCrossings, bool& partial)
    {
        if (fromEntity.IsValid() && toEntity.IsValid())
        {
            AZ::Vector3 start = AZ::Vector3::CreateZero(), end = AZ::Vector3::CreateZero();
            AZ::TransformBus::EventResult(start, fromEntity, &AZ::TransformBus::Events::GetWorldTranslation);
            AZ::TransformBus::EventResult(end, toEntity, &AZ::TransformBus::Events::GetWorldTranslation);

            return FindPathBetweenPositions(start, end, addCrossings, partial);
        }

        return {};
    }
    AZStd::vector<AZ::Vector3> EditorDetourNavigationComponent::FindPathBetweenPositions(
        const AZ::Vector3& fromWorldPosition, const AZ::Vector3& toWorldPosition, bool addCrossings, bool& partial)
    {
        AZStd::shared_ptr<NavMeshQuery> navMeshQuery;
        RecastNavigationMeshRequestBus::EventResult(navMeshQuery, m_navQueryEntityId, &RecastNavigationMeshRequests::GetNavigationObject);
        if (!navMeshQuery)
        {
            return {};
        }

        NavMeshQuery::LockGuard lock(*navMeshQuery);
        if (!lock.GetNavQuery())
        {
            return {};
        }

        RecastVector3 startRecast = RecastVector3::CreateFromVector3SwapYZ(fromWorldPosition);
        RecastVector3 endRecast = RecastVector3::CreateFromVector3SwapYZ(toWorldPosition);
        const float halfExtents[3] = { m_nearestDistance, m_nearestDistance, m_nearestDistance };

        dtPolyRef startPoly = 0, endPoly = 0;

        RecastVector3 nearestStartPoint, nearestEndPoint;

        const dtQueryFilter filter;

        // Find nearest points on the navigation mesh given the positions provided.
        // We are allowing some flexibility where looking for a point just a bit outside of the navigation mesh would still work.
        dtStatus result = lock.GetNavQuery()->findNearestPoly(startRecast.GetData(), halfExtents, &filter, &startPoly, nearestStartPoint.GetData());
        if (dtStatusFailed(result) || startPoly == 0)
        {
            return {};
        }

        result = lock.GetNavQuery()->findNearestPoly(endRecast.GetData(), halfExtents, &filter, &endPoly, nearestEndPoint.GetData());
        if (dtStatusFailed(result) || endPoly == 0)
        {
            return {};
        }

        // Some reasonable amount of waypoints along the path. Recast isn't made to calculate very long paths.
        constexpr int MaxPathLength = 100;

        AZStd::array<dtPolyRef, MaxPathLength> path;
        int pathLength = 0;

        // Find an approximate path first. In Recast, an approximate path is a collection of polygons, where a polygon covers an area.
        result = lock.GetNavQuery()->findPath(
            startPoly, endPoly, nearestStartPoint.GetData(), nearestEndPoint.GetData(), &filter, path.data(), &pathLength, MaxPathLength);
        if (dtStatusFailed(result))
        {
            return {};
        }
        partial = ((result & DT_PARTIAL_RESULT) != 0);

        AZStd::array<RecastVector3, MaxPathLength> detailedPath;
        AZStd::array<AZ::u8, MaxPathLength> detailedPathFlags;
        AZStd::array<dtPolyRef, MaxPathLength> detailedPolyPathRefs;
        int detailedPathCount = 0;

        // Then the detailed path. This gives us actual specific waypoints along the path over the polygons found earlier.
        result = lock.GetNavQuery()->findStraightPath(
            startRecast.GetData(),
            endRecast.GetData(),
            path.data(),
            pathLength,
            detailedPath[0].GetData(),
            detailedPathFlags.data(),
            detailedPolyPathRefs.data(),
            &detailedPathCount,
            MaxPathLength,
            addCrossings ? DT_STRAIGHTPATH_ALL_CROSSINGS : 0);

        if (dtStatusFailed(result))
        {
            return {};
        }

        AZStd::vector<AZ::Vector3> pathPoints;
        pathPoints.reserve(detailedPathCount);
        // Note: Recast uses +Y, O3DE used +Z as up vectors.
        for (int i = 0; i < detailedPathCount; ++i)
        {
            pathPoints.push_back(detailedPath[i].AsVector3WithZup());
        }

        return pathPoints;
    }
#endif
} // namespace RecastNavigation
