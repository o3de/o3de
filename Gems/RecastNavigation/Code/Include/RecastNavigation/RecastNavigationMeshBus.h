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

        virtual void UpdateNavigationMesh() = 0;

        virtual AZStd::vector<AZ::Vector3> FindPathToEntity( AZ::EntityId fromEntity, AZ::EntityId toEntity ) = 0;
        virtual AZStd::vector<AZ::Vector3> FindPathToPosition( const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition ) = 0;
    };

    using RecastNavigationMeshRequestBus = AZ::EBus<RecastNavigationMeshRequests>;

    class RecastNavigationMeshNotifications
        : public AZ::ComponentBus
    {
    public:

        virtual void OnNavigationMeshUpdated() = 0;
    };

    using RecastNavigationMeshNotificationBus = AZ::EBus<RecastNavigationMeshNotifications>;

} // namespace RecastNavigation
