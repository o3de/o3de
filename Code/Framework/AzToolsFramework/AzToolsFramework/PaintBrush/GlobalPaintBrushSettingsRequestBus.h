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
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettings.h>

namespace AzToolsFramework
{
    //! This is used to get/set the global paintbrush settings.
    class GlobalPaintBrushSettingsRequests : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        //! Returns a pointer to the current paintbrush settings.
        //! This shouldn't normally be used, but it's necessary for exposing the global paintbrush settings
        //! outwards to the Property Editor window.
        //! @return The paintbrush settings pointer
        virtual GlobalPaintBrushSettings* GetSettingsPointerForPropertyEditor() = 0;

        //! Returns a copy of the current global paintbrush settings
        virtual GlobalPaintBrushSettings GetSettings() const = 0;

        //! Returns the current brush mode for the paint brush settings
        virtual PaintBrushMode GetBrushMode() const = 0;

        //! Sets the brush mode for the paint brush settings.
        virtual void SetBrushMode(PaintBrushMode brushMode) = 0;

        //! Returns the current color mode for the paint brush settings
        virtual PaintBrushColorMode GetBrushColorMode() const = 0;

        //! Sets the color mode for the paint brush settings.
        virtual void SetBrushColorMode(PaintBrushColorMode colorMode) = 0;

        // Paint Brush Stroke settings

        //! Returns the current brush stroke color, including opacity.
        //! In monochrome painting, the RGB values will all be identical.
        virtual AZ::Color GetColor() const = 0;

        //! Returns the current brush stroke blend mode.
        virtual AzFramework::PaintBrushBlendMode GetBlendMode() const = 0;

        //! Returns the current brush stroke smooth mode.
        virtual AzFramework::PaintBrushSmoothMode GetSmoothMode() const = 0;

        //! Sets the brush stroke blend mode.
        //! @param blendMode The new blend mode.
        virtual void SetBlendMode(AzFramework::PaintBrushBlendMode blendMode) = 0;

        //! Sets the brush stroke smooth mode.
        //! @param smoothMode The new smooth mode.
        virtual void SetSmoothMode(AzFramework::PaintBrushSmoothMode smoothMode) = 0;

        //! Set the brush stroke color, including opacity.
        //! @param color The new brush color. In monochrome painting, only the Red value will be used.
        virtual void SetColor(const AZ::Color& color) = 0;

        // Paint Brush Stamp settings

        //! Returns the brush stamp size (diameter).
        //! @return The size of the paintbrush in meters
        virtual float GetSize() const = 0;

        //! Returns the brush stamp min/max size range.
        //! The range is used to ensure that our brush size is appropriately sized relative to the world size of the data we're painting.
        //! If we let it get too big, we can run into serious performance issues.
        //! @return A pair containing the min size and max size that constrain the range of sizes in meters for the paintbrush.
        virtual AZStd::pair<float, float> GetSizeRange() const = 0;

        //! Returns the brush stamp hardness (0=soft falloff, 100=hard edge).
        virtual float GetHardnessPercent() const = 0;

        //! Returns the brush stamp flow setting (0=transparent stamp, 100=opaque stamp)
        virtual float GetFlowPercent() const = 0;

        //! Returns the brush distance to move between each stamp placement in % of paintbrush size.
        virtual float GetDistancePercent() const = 0;

        //! Sets the brush stamp size (diameter).
        //! @param size The new size, in meters.
        virtual void SetSize(float size) = 0;

        //! Sets the brush stamp min/max size range.
        //! The range is used to ensure that our brush size is appropriately sized relative to the world size of the data we're painting.
        //! If we let it get too big, we can run into serious performance issues.
        //! @param minSize The minimum size of the paint brush in meters.
        //! @param maxSize The maximum size of the paint brush in meters.
        virtual void SetSizeRange(float minSize, float maxSize) = 0;

        //! Sets the brush stamp hardness.
        //! @param hardness The new hardness, in 0-100 range.
        virtual void SetHardnessPercent(float hardnessPercent) = 0;

        //! Sets the brush stamp flow setting.
        //! @param flow The new flow, in 0-100 range.
        virtual void SetFlowPercent(float flowPercent) = 0;

        //! Set the brush distance % to move between each stamp placement.
        //! @param distancePercent The new distance %, typically in the 0-100 range.
        virtual void SetDistancePercent(float distancePercent) = 0;

        // Paint Brush Smoothing settings

        //! Returns the number of pixels in each direction to use for smoothing calculations.
        //! @return The number of pixels in each direction to use
        virtual size_t GetSmoothingRadius() const = 0;

        //! Returns the number of pixels to skip between pixel fetches for smoothing calculations.
        //! @return The number of pixels to skip
        virtual size_t GetSmoothingSpacing() const = 0;

        //! Set the number of pixels in each direction to use for smoothing calculations.
        //! @param The number of pixels in each direction to use
        virtual void SetSmoothingRadius(size_t radius) = 0;

        //! Set the number of pixels to skip between pixel fetches for smoothing calculations
        //! @param The number of pixels to skip
        virtual void SetSmoothingSpacing(size_t spacing) = 0;

    protected:
        ~GlobalPaintBrushSettingsRequests() = default;
    };

    using GlobalPaintBrushSettingsRequestBus = AZ::EBus<GlobalPaintBrushSettingsRequests>;

} // namespace AzToolsFramework
