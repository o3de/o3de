/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AtomToolsFramework
{
    //! Provides an interface for providing notifications specific to RenderViewportWidget.
    //! @note Most behaviors in RenderViewportWidget are handled by its underyling
    //! ViewportContext, this bus is specifically for functionality exclusive to the
    //! Qt layer provided by RenderViewportWidget.
    class RenderViewportWidgetNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Triggered when the idle frame rate limit for inactive viewports changed.
        //! Controlled by the ed_inactive_viewport_fps_limit CVAR.
        //! Active viewports are controlled by the r_fps_limit CVAR.
        virtual void OnInactiveViewportFrameRateChanged([[maybe_unused]]float fpsLimit){}

    protected:
        ~RenderViewportWidgetNotifications() = default;
    };

    using RenderViewportWidgetNotificationBus = AZ::EBus<RenderViewportWidgetNotifications>;
} // namespace AtomToolsFramework
