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
#include <AzCore/std/containers/span.h>

#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    //! PaintBrushSettingsRequestBus is used to get/set the global paintbrush settings
    class PaintBrushSettingsRequests : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        //! GetRadius returns the current paintbrush radius.
        //! @return The radius of the paintbrush
        virtual float GetRadius() const = 0;

        //! GetIntensity returns the current paintbrush intensity (0=black, 1=white).
        //! @return The intensity of the paintbrush
        virtual float GetIntensity() const = 0;

        //! GetOpacity returns the current paintbrush opacity (0=transparent, 1=opaque).
        virtual float GetOpacity() const = 0;

        //! SetRadius sets the paintbrush radius.
        //! @param radius The new radius, in meters.
        virtual void SetRadius(float radius) = 0;

        //! SetIntensity sets the paintbrush intensity.
        //! @param intensity The new intensity, in 0-1 range (0=black, 1=white).
        virtual void SetIntensity(float intensity) = 0;

        //! SetOpacity sets the paintbrush opacity.
        //! @param opacity The new opacity, in 0-1 range (0=transparent, 1=opaque).
        virtual void SetOpacity(float opacity) = 0;

    protected:
        ~PaintBrushSettingsRequests() = default;
    };

    using PaintBrushSettingsRequestBus = AZ::EBus<PaintBrushSettingsRequests>;

    //! PaintBrushSettingsNotificationBus is used to send out notifications whenever the global paintbrush settings have changed.
    class PaintBrushSettingsNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        // This uses a Broadcast where each message has an EntityComponentIdPair instead of having the EBus use that as an ID
        // because the PaintBrushSettings window will want to listen to all notifications, regardless of which entity they came from.

        //! OnIntensityChanged notifies listeners that the paintbrush intensity setting has changed.
        //! @param intensity The new intensity setting for the paintbrush (0=black, 1=white).
        virtual void OnIntensityChanged([[maybe_unused]] float intensity)
        {
        }

        //! OnOpacityChanged notifies listeners that the paintbrush opacity setting has changed.
        //! @param opacity The new opacity setting for the paintbrush (0=transparent, 1=opaque).
        virtual void OnOpacityChanged([[maybe_unused]] float opacity)
        {
        }

        //! OnRadiusChanged notifies listeners that the paintbrush radius setting has changed.
        //! @param radius The new radius setting for the paintbrush, in meters.
        virtual void OnRadiusChanged([[maybe_unused]] float radius)
        {
        }
    };

    using PaintBrushSettingsNotificationBus = AZ::EBus<PaintBrushSettingsNotifications>;

} // namespace AzToolsFramework
