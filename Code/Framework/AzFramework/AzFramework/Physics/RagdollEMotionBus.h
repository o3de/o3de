/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace Physics
{
    class RagdollEMotionNotifications : public AZ::EBusTraits
    {
    public:
        virtual void OnRagdollPropertiesConfigurationChanged() = 0;
        virtual void OnRagdollJointLimitConfigurationChanged() = 0;
        virtual void OnRagdollColliderConfigurationChanged() = 0;
    };

    using RagdollEMotionNotificationBus = AZ::EBus<RagdollEMotionNotifications>;
}
