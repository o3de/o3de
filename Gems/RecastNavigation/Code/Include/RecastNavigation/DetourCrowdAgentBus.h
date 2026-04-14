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

namespace RecastNavigation
{
    //! Notification interface for per-agent crowd simulation events.
    class DetourCrowdAgentNotifications
        : public AZ::ComponentBus
    {
    public:
        //! Notifies that an agent position has been updated by crowd simulation.
        virtual void OnAgentPositionUpdated(const AZ::Vector3& worldPosition, const AZ::Vector3& worldVelocity) = 0;
    };

    //! Notification EBus addressed by agent entity id.
    using DetourCrowdAgentNotificationBus = AZ::EBus<DetourCrowdAgentNotifications>;
} // namespace RecastNavigation
