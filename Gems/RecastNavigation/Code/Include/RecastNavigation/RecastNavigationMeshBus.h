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
    class RecastNavigationMeshRequests
        : public AZ::ComponentBus
    {
    public:
        //! Re-calculates the navigation mesh withing the defined world area.
        virtual void UpdateNavigationMesh() = 0;
        
        //! Blocking call that find a walkable path between two entities.
        //! @param fromEntity The starting point of the path from the position of this entity.
        //! @param toEntity The end point of the path is at the position of this entity.
        //! @return If a path is found, returns a vector of waypoints.
        virtual AZStd::vector<AZ::Vector3> FindPathToEntity( AZ::EntityId fromEntity, AZ::EntityId toEntity ) = 0;
        
        //! Blocking call that find a walkable path between two world positions.
        //! @param fromWorldPosition The starting point of the path.
        //! @param toWorldPosition The end point of the path to find.
        //! @return If a path is found, returns a vector of waypoints.
        virtual AZStd::vector<AZ::Vector3> FindPathToPosition( const AZ::Vector3& fromWorldPosition, const AZ::Vector3& toWorldPosition ) = 0;
        
        //! @return returns the underlying navigation map object
        virtual dtNavMesh* GetNativeNavigationMap() const = 0;
        
        //! @return returns the underlying navigation query object
        virtual dtNavMeshQuery* GetNativeNavigationQuery() const = 0;
    };

    using RecastNavigationMeshRequestBus = AZ::EBus<RecastNavigationMeshRequests>;

    class RecastNavigationMeshNotifications
        : public AZ::ComponentBus
    {
    public:        
        //! Notifies when a navigation mesh is re-calculated and updated.
        virtual void OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity) = 0;
    };

    using RecastNavigationMeshNotificationBus = AZ::EBus<RecastNavigationMeshNotifications>;

    //! Scripting reflection helper for @RecastNavigationMeshNotificationBus
    class RecastNavigationNotificationHandler
        : public RecastNavigationMeshNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(RecastNavigationNotificationHandler,
            "{819FF083-C28A-4620-B59E-78EB7D2CB432}", 
            AZ::SystemAllocator, OnNavigationMeshUpdated);

        void OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity) override
        {
            Call(FN_OnNavigationMeshUpdated, navigationMeshEntity);
        }
    };
} // namespace RecastNavigation
