/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DetourNavMesh.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Misc/RecastSmartPointer.h>

namespace RecastNavigation
{
    //! Holds a pointer to Recast navigation mesh and the associated mutex.
    //! This structure is to be used when performing operations on a navigation mesh.
    struct NavMeshQuery
    {
        //! Recast navigation mesh object.
        RecastPointer<dtNavMesh> m_mesh;

        //! Recast navigation query object can be used to find paths.
        RecastPointer<dtNavMeshQuery> m_query;
    };

    class RecastNavigationMeshRequests
        : public AZ::ComponentBus
    {
    public:
        //! Re-calculates the navigation mesh withing the defined world area. Blocking call.
        virtual void UpdateNavigationMeshBlockUntilCompleted() = 0;
        
        //! @return returns the underlying navigation objects with the associated synchronization object
        virtual AZStd::shared_ptr<NavMeshQuery> GetNavigationObject() = 0;
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
