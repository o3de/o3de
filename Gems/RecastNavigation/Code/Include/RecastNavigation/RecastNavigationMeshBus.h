/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>

class dtNavMeshQuery;
class dtNavMesh;

namespace RecastNavigation
{
    class RecastNavigationMeshRequests
        : public AZ::ComponentBus
    {
    public:

        virtual bool UpdateNavigationMesh() = 0;
        virtual void SetWorldBounds( const AZ::Aabb& worldBounds ) = 0;

        virtual AZStd::vector<AZ::Vector3> FindPathToEntity( AZ::EntityId fromEntity, AZ::EntityId toEntity ) = 0;
        virtual AZStd::vector<AZ::Vector3> FindPathToPosition( const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition ) = 0;
    };

    using RecastNavigationMeshRequestBus = AZ::EBus<RecastNavigationMeshRequests>;

    class RecastNavigationMeshNotifications
        : public AZ::ComponentBus
    {
    public:

        virtual void OnNavigationMeshUpdated( dtNavMesh* updatedNavMesh, dtNavMeshQuery* updatedNavMeshQuery ) = 0;
    };

    using RecastNavigationMeshNotificationBus = AZ::EBus<RecastNavigationMeshNotifications>;

    class RecastWalkableRequests
        : public AZ::ComponentBus
    {
    public:

        virtual bool IsWalkable( AZ::EntityId navigationMeshEntity ) = 0;
    };

    // Use to mark objects as walkable
    using RecastWalkableRequestBus = AZ::EBus<RecastWalkableRequests>;

} // namespace RecastNavigation
