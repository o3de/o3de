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
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettings.h>

namespace AzToolsFramework
{
    //! This is used to get/set the global paintbrush settings.
    class PaintBrushSettingsRequests : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        //! Returns all of the current paintbrush settings.
        //! This shouldn't normally be used, but it's necessary for exposing the global paintbrush settings
        //! outwards to the Property Editor window.
        //! @return The paintbrush settings
        virtual PaintBrushSettings* GetSettings() = 0;

        //! Returns the current paintbrush radius.
        //! @return The radius of the paintbrush
        virtual float GetRadius() const = 0;

        //! Returns the current paintbrush intensity (0=black, 1=white).
        //! @return The intensity of the paintbrush
        virtual float GetIntensity() const = 0;

        //! Returns the current paintbrush opacity (0=transparent, 1=opaque).
        virtual float GetOpacity() const = 0;

        //! Returns the current paintbrush blend mode.
        virtual PaintBrushBlendMode GetBlendMode() const = 0;

        //! Sets the paintbrush radius.
        //! @param radius The new radius, in meters.
        virtual void SetRadius(float radius) = 0;

        //! Sets the paintbrush intensity.
        //! @param intensity The new intensity, in 0-1 range (0=black, 1=white).
        virtual void SetIntensity(float intensity) = 0;

        //! Sets the paintbrush opacity.
        //! @param opacity The new opacity, in 0-1 range (0=transparent, 1=opaque).
        virtual void SetOpacity(float opacity) = 0;

        //! Sets the paintbrush blend mode.
        //! @param blendMode The new blend mode.
        virtual void SetBlendMode(PaintBrushBlendMode blendMode) = 0;

    protected:
        ~PaintBrushSettingsRequests() = default;
    };

    using PaintBrushSettingsRequestBus = AZ::EBus<PaintBrushSettingsRequests>;

} // namespace AzToolsFramework
