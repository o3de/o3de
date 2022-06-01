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

        //! Recast navigation query object that can be used to find paths.
        RecastPointer<dtNavMeshQuery> m_query;
    };

    //! The interface for request API of @RecastNavigationMeshRequestBus.
    class RecastNavigationMeshRequests
        : public AZ::ComponentBus
    {
    public:
        //! Re-calculates the navigation mesh within the defined world area. Blocking call.
        virtual void UpdateNavigationMeshBlockUntilCompleted() = 0;

        //! @returns the underlying navigation objects with the associated synchronization object.
        virtual AZStd::shared_ptr<NavMeshQuery> GetNavigationObject() = 0;
    };

    //! Request EBus for a navigation mesh component.
    using RecastNavigationMeshRequestBus = AZ::EBus<RecastNavigationMeshRequests>;

    //! The interface for notification API of @RecastNavigationMeshNotificationBus.
    class RecastNavigationMeshNotifications
        : public AZ::ComponentBus
    {
    public:
        //! Notifies when a navigation mesh is re-calculated and updated.
        //! @param navigationMeshEntity the entity the navigation mesh is on. This is helpful for Script Canvas use.
        virtual void OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity) = 0;
    };

    //! Notification EBus for a navigation mesh component.
    using RecastNavigationMeshNotificationBus = AZ::EBus<RecastNavigationMeshNotifications>;

    //! Scripting reflection helper for @RecastNavigationMeshNotificationBus.
    class RecastNavigationNotificationHandler
        : public RecastNavigationMeshNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(RecastNavigationNotificationHandler,
            "{819FF083-C28A-4620-B59E-78EB7D2CB432}",
            AZ::SystemAllocator, OnNavigationMeshUpdated);

        //! Notifies when a navigation mesh is updated.
        //! @param navigationMeshEntity the entity with the navigation mesh. This makes for an easier to work in Script Cavnas.
        void OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity) override
        {
            Call(FN_OnNavigationMeshUpdated, navigationMeshEntity);
        }
    };
} // namespace RecastNavigation
