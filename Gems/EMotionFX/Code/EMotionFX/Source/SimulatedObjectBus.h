/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace EMotionFX
{
    class SimulatedObject;

    class SimulatedObjectNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual void OnSimulatedObjectChanged() {}
    };

    using SimulatedObjectNotificationBus = AZ::EBus<SimulatedObjectNotifications>;
} // namespace EMotionFX
