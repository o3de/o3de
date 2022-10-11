/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    enum class PaintBrushBlendMode : uint8_t;

    //! PaintBrushSettingsNotificationBus is used to send out notifications whenever the global paintbrush settings have changed.
    class PaintBrushSettingsNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Notifies listeners that the paintbrush intensity setting has changed.
        //! @param intensity The new intensity setting for the paintbrush (0=black, 1=white).
        virtual void OnIntensityChanged([[maybe_unused]] float intensity)
        {
        }

        //! Notifies listeners that the paintbrush opacity setting has changed.
        //! @param opacity The new opacity setting for the paintbrush (0=transparent, 1=opaque).
        virtual void OnOpacityChanged([[maybe_unused]] float opacity)
        {
        }

        //! Notifies listeners that the paintbrush radius setting has changed.
        //! @param radius The new radius setting for the paintbrush, in meters.
        virtual void OnRadiusChanged([[maybe_unused]] float radius)
        {
        }

        //! Notifies listeners that the paintbrush radius setting has changed.
        //! @param radius The new radius setting for the paintbrush, in meters.
        virtual void OnBlendModeChanged([[maybe_unused]] PaintBrushBlendMode blendMode)
        {
        }
    };

    using PaintBrushSettingsNotificationBus = AZ::EBus<PaintBrushSettingsNotifications>;

} // namespace AzToolsFramework
