/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace RecastNavigation
{
    //! Notification interface for per-agent crowd simulation events.
    class DetourCrowdAgentNotifications : public AZ::ComponentBus
    {
    public:
        //! Notifies that an agent position has been updated by crowd simulation.
        virtual void OnAgentPositionUpdated(const AZ::Vector3& worldPosition, const AZ::Vector3& worldVelocity) = 0;
    };

    //! Notification EBus addressed by agent entity id.
    using DetourCrowdAgentNotificationBus = AZ::EBus<DetourCrowdAgentNotifications>;

    //! Scripting reflection helper for @DetourCrowdAgentNotificationBus.
    class DetourCrowdAgentNotificationHandler
        : public DetourCrowdAgentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            DetourCrowdAgentNotificationHandler, "{AF6C3767-8ADD-45AD-8086-67ACF732E7C4}", AZ::SystemAllocator, OnAgentPositionUpdated);

        void OnAgentPositionUpdated(const AZ::Vector3& worldPosition, const AZ::Vector3& worldVelocity) override
        {
            Call(FN_OnAgentPositionUpdated, worldPosition, worldVelocity);
        }
    };
} // namespace RecastNavigation
