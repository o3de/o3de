/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace RecastNavigation
{
    //! Interface for path finding API.
    class DetourNavigationRequests
        : public AZ::ComponentBus
    {
    public:
        //! An entity with a navigation mesh is required to calculate paths.
        //! @param navMeshEntity an entity with @RecastNavigationMeshComponent
        virtual void SetNavigationMeshEntity(AZ::EntityId navMeshEntity) = 0;

        //! An entity with a navigation mesh is required to calculate paths.
        //! @return the associated entity with @RecastNavigationMeshComponent
        virtual AZ::EntityId GetNavigationMeshEntity() const = 0;

        //! Blocking call that finds a walkable path between two entities.
        //! @param fromEntity The starting point of the path from the position of this entity.
        //! @param toEntity The end point of the path is at the position of this entity.
        //! @return If a path is found, returns a vector of waypoints. An empty vector is returned if a path was not found.
        virtual AZStd::vector<AZ::Vector3> FindPathBetweenEntities(AZ::EntityId fromEntity, AZ::EntityId toEntity) = 0;

        //! Blocking call that finds a walkable path between two world positions.
        //! @param fromWorldPosition The starting point of the path.
        //! @param toWorldPosition The end point of the path to find.
        //! @return If a path is found, returns a vector of waypoints. An empty vector is returned if a path was not found.
        virtual AZStd::vector<AZ::Vector3> FindPathBetweenPositions(const AZ::Vector3& fromWorldPosition, const AZ::Vector3& toWorldPosition) = 0;
    };

    //! Request EBus for a path finding component.
    using DetourNavigationRequestBus = AZ::EBus<DetourNavigationRequests>;
} // namespace RecastNavigation
