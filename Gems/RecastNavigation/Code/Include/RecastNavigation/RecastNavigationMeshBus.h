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
    //! In order to access NavMesh or NavMeshQuery objects, use the object LockGuard(NavMeshQuery&)
    class NavMeshQuery
    {
    public:
        class LockGuard;

        NavMeshQuery(dtNavMesh* navMesh, dtNavMeshQuery* navQuery)
        {
            m_mesh.reset(navMesh);
            m_query.reset(navQuery);
        }

        //! A lock guard class with accessors for navigation mesh and query objects.
        //! A lock is held in place until this object goes out of scope.
        //! Release this object as soon as you are done working with the navigation mesh.
        class LockGuard
        {
        public:
            //! Grabs a lock on a mutex in @NavMeshQuery
            //! @param navMesh navigation mesh to hold on to
            explicit LockGuard(NavMeshQuery& navMesh)
                : m_lock(navMesh.m_mutex)
                , m_mesh(navMesh.m_mesh.get())
                , m_query(navMesh.m_query.get())
            {
            }

            //! Navigation mesh accessor.
            dtNavMesh* GetNavMesh()
            {
                return m_mesh;
            }

            //! Navigation mesh query accessor.
            dtNavMeshQuery* GetNavQuery()
            {
                return m_query;
            }

        private:
            AZStd::lock_guard<AZStd::recursive_mutex> m_lock;
            dtNavMesh* m_mesh = nullptr;
            dtNavMeshQuery* m_query = nullptr;

            AZ_DISABLE_COPY_MOVE(LockGuard);
        };

    private:
        //! Recast navigation mesh object.
        RecastPointer<dtNavMesh> m_mesh;

        //! Recast navigation query object that can be used to find paths.
        RecastPointer<dtNavMeshQuery> m_query;

        //! A mutex for accessing and modifying the navigation mesh.
        AZStd::recursive_mutex m_mutex;
    };

    //! The interface for request API of @RecastNavigationMeshRequestBus.
    class RecastNavigationMeshRequests
        : public AZ::ComponentBus
    {
    public:
        //! Re-calculates the navigation mesh within the defined world area. Blocking call.
        virtual void UpdateNavigationMeshBlockUntilCompleted() = 0;

        //! Re-calculates the navigation mesh within the defined world area. Notifies when completed using @RecastNavigationMeshNotificationBus.
        virtual void UpdateNavigationMeshAsync() = 0;

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
