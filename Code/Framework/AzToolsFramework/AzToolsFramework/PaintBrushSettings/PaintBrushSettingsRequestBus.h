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

        //! Returns a pointer to the current paintbrush settings.
        //! This shouldn't normally be used, but it's necessary for exposing the global paintbrush settings
        //! outwards to the Property Editor window.
        //! @return The paintbrush settings pointer
        virtual PaintBrushSettings* GetSettingsPointerForPropertyEditor() = 0;

        //! Returns a copy of the current paintbrush settings
        virtual PaintBrushSettings GetSettings() const = 0;

        // Paint Brush Stroke settings

        //! Returns the brush stroke intensity (0=black, 100=white).
        //! @return The intensity of the brush stroke
        virtual float GetIntensityPercent() const = 0;

        //! Returns the brush stroke opacity (0=transparent, 100=opaque).
        virtual float GetOpacityPercent() const = 0;

        //! Returns the current brush stroke blend mode.
        virtual PaintBrushBlendMode GetBlendMode() const = 0;

        //! Sets the brush stroke intensity.
        //! @param intensity The new intensity, in 0-100 range (0=black, 100=white).
        virtual void SetIntensityPercent(float intensityPercent) = 0;

        //! Sets the brush stroke opacity.
        //! @param opacity The new opacity, in 0-100 range (0=transparent, 100=opaque).
        virtual void SetOpacityPercent(float opacityPercent) = 0;

        //! Sets the brush stroke blend mode.
        //! @param blendMode The new blend mode.
        virtual void SetBlendMode(PaintBrushBlendMode blendMode) = 0;

        // Paint Brush Stamp settings

        //! Returns the brush stamp size (diameter).
        //! @return The size of the paintbrush in meters
        virtual float GetSize() const = 0;

        //! Returns the brush stamp hardness (0=soft falloff, 100=hard edge).
        virtual float GetHardnessPercent() const = 0;

        //! Returns the brush stamp flow setting (0=transparent stamp, 100=opaque stamp)
        virtual float GetFlowPercent() const = 0;

        //! Returns the brush distance to move between each stamp placement in % of paintbrush size.
        virtual float GetDistancePercent() const = 0;

        //! Sets the brush stamp size (diameter).
        //! @param size The new size, in meters.
        virtual void SetSize(float size) = 0;

        //! Sets the brush stamp hardness.
        //! @param hardness The new hardness, in 0-100 range.
        virtual void SetHardnessPercent(float hardnessPercent) = 0;

        //! Sets the brush stamp flow setting.
        //! @param flow The new flow, in 0-100 range.
        virtual void SetFlowPercent(float flowPercent) = 0;

        //! Set the brush distance % to move between each stamp placement.
        //! @param distancePercent The new distance %, typically in the 0-100 range.
        virtual void SetDistancePercent(float distancePercent) = 0;

    protected:
        ~PaintBrushSettingsRequests() = default;
    };

    using PaintBrushSettingsRequestBus = AZ::EBus<PaintBrushSettingsRequests>;

} // namespace AzToolsFramework
