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

        virtual void UpdateNavigationMesh() = 0;

        virtual AZStd::vector<AZ::Vector3> FindPathToEntity( AZ::EntityId fromEntity, AZ::EntityId toEntity ) = 0;
        virtual AZStd::vector<AZ::Vector3> FindPathToPosition( const AZ::Vector3& fromWorldPosition, const AZ::Vector3& targetWorldPosition ) = 0;
    };

    using RecastNavigationMeshRequestBus = AZ::EBus<RecastNavigationMeshRequests>;

    class RecastNavigationMeshNotifications
        : public AZ::ComponentBus
    {
    public:

        virtual void OnNavigationMeshUpdated(AZ::EntityId navigationMeshEntity) = 0;
    };

    using RecastNavigationMeshNotificationBus = AZ::EBus<RecastNavigationMeshNotifications>;

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
