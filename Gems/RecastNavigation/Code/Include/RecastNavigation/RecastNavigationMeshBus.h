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
#include <RecastNavigation/NavMeshQuery.h>
#include <RecastNavigation/RecastSmartPointer.h>

namespace RecastNavigation
{
    //! The interface for request API of @RecastNavigationMeshRequestBus.
    class RecastNavigationMeshRequests
        : public AZ::ComponentBus
    {
    public:
        //! Re-calculates the navigation mesh within the defined world area. Blocking call.
        //! @returns false if another update operation is already in progress
        virtual bool UpdateNavigationMeshBlockUntilCompleted() = 0;

        //! Re-calculates the navigation mesh within the defined world area. Notifies when completed using @RecastNavigationMeshNotificationBus.
        //! @returns false if another update operation is already in progress
        virtual bool UpdateNavigationMeshAsync() = 0;

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

        //! Notifies when a navigation mesh has started to re-calculate the navigation mesh.
        //! @param navigationMeshEntity the entity the navigation mesh is on. This is helpful for Script Canvas use.
        virtual void OnNavigationMeshBeganRecalculating(AZ::EntityId navigationMeshEntity) = 0;
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
            AZ::SystemAllocator, OnNavigationMeshUpdated, OnNavigationMeshBeganRecalculating);

        //! Notifies when a navigation mesh is updated.
        //! @param navigationMeshEntity the entity with the navigation mesh. This makes for an easier to work in Script Cavnas.
        void OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity) override
        {
            Call(FN_OnNavigationMeshUpdated, navigationMeshEntity);
        }

        //! Notifies when a navigation mesh has started to re-calculated navigation tile data.
        //! @param navigationMeshEntity the entity the navigation mesh is on. This is helpful for Script Canvas use.
        void OnNavigationMeshBeganRecalculating(AZ::EntityId navigationMeshEntity) override
        {
            Call(FN_OnNavigationMeshBeganRecalculating, navigationMeshEntity);
        }
    };
} // namespace RecastNavigation
