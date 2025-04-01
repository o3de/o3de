/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DetourNavMesh.h>
#include <RecastNavigation/RecastSmartPointer.h>

namespace RecastNavigation
{
    //! Holds pointers to Recast navigation mesh objects and the associated mutex.
    //! This structure should be used when performing operations on a navigation mesh.
    //! In order to access NavMesh or NavMeshQuery objects, use the object LockGuard(NavMeshQuery&).
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
} // namespace RecastNavigation
