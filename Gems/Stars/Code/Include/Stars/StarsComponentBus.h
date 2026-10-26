/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::Render
{
    class StarsComponentRequests : public AZ::ComponentBus
    {
    public:
        ~StarsComponentRequests() override = default;

        virtual float GetStarsExposure() const = 0;
        virtual void SetStarsExposure(float exposure) = 0;
        virtual float GetStarsRadiusFactor() const = 0;
        virtual void SetStarsRadiusFactor(float radiusFactor) = 0;
        virtual float GetStarsTwinkleRate() const = 0;
        virtual void SetStarsTwinkleRate(float twinkleRate) = 0;
    };

    using StarsComponentRequestBus = AZ::EBus<StarsComponentRequests>;

    class StarsComponentNotifications : public AZ::ComponentBus
    {
    };

    using StarsComponentNotificationBus = AZ::EBus<StarsComponentNotifications>;

    class StarsComponentNotificationHandler
        : public StarsComponentNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    };
} // namespace AZ::Render
