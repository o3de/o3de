/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Serialization/SerializeContext.h>

// This needs to be forward-declared so that we can make it a friend class.
namespace AzToolsFramework
{
    class GlobalPaintBrushSettings;
}

namespace AzFramework
{
    /*
       Paint brushes have multiple settings that all come together to produce the results that we're used to seeing in
       paint programs. For those who aren't familiar, here's a quick explanation of how it all works.

       Painting consists of "brush strokes", where each stroke consists of pushing a brush against a canvas, moving it, and lifting
       it. Our equivalent is holding down the mouse button, moving the mouse in continuous movements while holding down the button,
       then lifting the button at the end of the stroke. However, instead of a continuous brush stroke, paint programs use something
       closer to pointillism, where the stroke consists of many discrete brush "stamps", where a "stamp" is a single discrete imprint
       of the brush shape.

       With that description in mind, here's how the settings play together:

       Stroke settings:
       - Color: The color of the paint on the paintbrush. This color is fully applied to each brush stroke.
       - Intensity: When using greyscale painting, this replaces color with an intensity from black to white. This color is fully applied to
       each brush stroke.
       - Opacity: The opacity of an entire brush stroke. Because it's per-stroke, a stroke that crosses itself won't blend with
         itself when this opacity setting is lowered. The blending only occurs after the stroke is complete. (A stroke can still
         blend with itself from flow and hardness settings below, just not from the stroke opacity setting)

       - Blend Mode: The blend mode for blending the brush stroke down to the base layer.

       Stamp settings:
       - Size: The size of the paintbrush, which is the size of each stamp.
       - Distance: The amount the brush needs to move before leaving another stamp, expressed in a % of the paintbrush size.
         100% roughly means each stamp will have 0% overlap and be adjacent to each other, where 25% means each stamp will overlap
         by 75% while creating the stroke. If the brush moves back and forth in small amounts, it's possible to get higher amounts
         of overlap, since this is distance of brush movement, not distance between stamps.
       - Flow: The opacity of each stamp. This conceptually maps to the amount of paint flowing through an airbrush.
       - Hardness: The amount of falloff from the center of each stamp, which affects the opacity within each pixel of the stamp.
         This sort of represents how hard the brush is being pressed against the canvas.

       Each brush stroke conceptually occurs on a separate stroke layer that gets blended into the base, and the layer is merged down
       into the base at the end of the stroke. As the brush moves, it creates discrete stamps based on the Size and Distance. Each stamp
       paints per-pixel opacity data into the stroke layer based on the combination of Flow and Hardness. The stroke layer itself is
       then blended down using the Intensity, Opacity, and Blend Mode.
    */

    //! The different types of blend modes supported by the paint brush tool.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(PaintBrushBlendMode, uint8_t,
        (Normal, 0),    //!< Alpha blends between the paint brush value and the existing value 
        (Add, 1),       //!< Adds the paint brush value to the existing value
        (Subtract, 2),  //!< Subtracts the paint brush value from the existing value
        (Multiply, 3),  //!< Multiplies the paint brush value with the existing value. (Darkens)
        (Screen, 4),    //!< Inverts the two values, multiplies, then inverts again. (Lightens)
        (Darken, 5),    //!< Keeps the minimum of the paint brush value and the existing value
        (Lighten, 6),   //!< Keeps the maximum of the paint brush value and the existing value
        (Average, 7),   //!< Takes the average of the paint brush value and the existing value
        (Overlay, 8)    //!< Combines Multiply and Screen - darkens when brush < 0.5, lightens when brush > 0.5
    );

    //! The different types of smoothing modes supported by the paint brush tool.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(PaintBrushSmoothMode, uint8_t,
        (Gaussian, 0), 
        (Mean, 1), 
        (Median, 2)
    );

    //! Defines the specific paintbrush settings to use with a paintbrush.
    class PaintBrushSettings
    {
    public:

        // GlobalPaintBrushSettings is declared as a friend class even though it's a subclass so that it can have the permissions
        // to serialize an EditContext for PaintBrushSettings.
        friend class AzToolsFramework::GlobalPaintBrushSettings;

        AZ_CLASS_ALLOCATOR(PaintBrushSettings, AZ::SystemAllocator);
        AZ_TYPE_INFO(PaintBrushSettings, "{CE5EFFE2-14E5-4A9F-9B0F-695F66744A50}");
        static void Reflect(AZ::ReflectContext* context);

        PaintBrushSettings() = default;
        virtual ~PaintBrushSettings() = default;

        // Stroke settings

        AZ::Color GetColor() const
        {
            return m_brushColor;
        }

        PaintBrushBlendMode GetBlendMode() const
        {
            return m_blendMode;
        }

        PaintBrushSmoothMode GetSmoothMode() const
        {
            return m_smoothMode;
        }

        void SetColor(const AZ::Color& color);
        void SetBlendMode(PaintBrushBlendMode blendMode);
        void SetSmoothMode(PaintBrushSmoothMode smoothMode);

        // Stamp settings

        float GetSize() const
        {
            return m_size;
        }

        AZStd::pair<float, float> GetSizeRange() const
        {
            return { m_sizeMin, m_sizeMax };
        }

        float GetHardnessPercent() const
        {
            return m_hardnessPercent;
        }
        float GetFlowPercent() const
        {
            return m_flowPercent;
        }
        float GetDistancePercent() const
        {
            return m_distancePercent;
        }

        void SetSize(float size);
        void SetSizeRange(float minSize, float maxSize);
        void SetHardnessPercent(float hardnessPercent);
        void SetFlowPercent(float flowPercent);
        void SetDistancePercent(float distancePercent);

        // Smoothing settings

        size_t GetSmoothingRadius() const
        {
            return m_smoothingRadius;
        }
        size_t GetSmoothingSpacing() const
        {
            return m_smoothingSpacing;
        }

        void SetSmoothingRadius(size_t radius);
        void SetSmoothingSpacing(size_t spacing);

    protected:
        AZ::u32 OnColorChanged();
        AZ::u32 OnIntensityChanged();
        AZ::u32 OnOpacityChanged();

        virtual bool GetSizeVisibility() const;
        virtual bool GetColorVisibility() const;
        virtual bool GetIntensityVisibility() const;
        virtual bool GetOpacityVisibility() const;
        virtual bool GetHardnessVisibility() const;
        virtual bool GetFlowVisibility() const;
        virtual bool GetDistanceVisibility() const;
        virtual bool GetSmoothingRadiusVisibility() const;
        virtual bool GetBlendModeVisibility() const;
        virtual bool GetSmoothModeVisibility() const;

        float GetSizeMin() const;
        float GetSizeMax() const;
        float GetSizeStep() const;

        //! Notification functions for editing changes that aren't used for anything in PaintBrushSettings but can be overridden.
        //! They exist so that the GlobalPaintBrushSettings can notify listeners whenever the global settings change.
        virtual void OnSizeRangeChanged();
        virtual AZ::u32 OnSettingsChanged();

        //! Brush stroke color
        AZ::Color m_brushColor = AZ::Color::CreateOne();
        //! Brush stroke blend mode
        PaintBrushBlendMode m_blendMode = PaintBrushBlendMode::Normal;
        //! Brush stroke smooth mode
        PaintBrushSmoothMode m_smoothMode = PaintBrushSmoothMode::Gaussian;

        //! Brush stamp diameter in meters
        float m_size = 10.0f;
        float m_sizeMin = 0.0f;
        float m_sizeMax = 1024.0f;

        //! Brush stamp hardness percent (0=soft falloff, 100=hard edge)
        float m_hardnessPercent = 100.0f;
        //! Brush stamp flow percent (0=transparent stamps, 100=opaque stamps)
        float m_flowPercent = 100.0f;
        //! Brush distance to move between stamps in % of paintbrush size. (25% is the default in Photoshop.)
        float m_distancePercent = 25.0f;

        static inline constexpr size_t MinSmoothingRadius = 1;  // We need to use at least 1 pixel in each direction for smoothing
        static inline constexpr size_t MaxSmoothingRadius = 10; // Limit the max to 10 pixels in each direction due to performance

        //! Brush smoothing radius - the number of pixels in each direction to use for smoothing calculations.
        size_t m_smoothingRadius = MinSmoothingRadius;

        static inline constexpr size_t MinSmoothingSpacing = 0;     // A spacing of 0 means use adjacent pixels
        static inline constexpr size_t MaxSmoothingSpacing = 50;    // Reasonable limit that doesn't spread the pixels too far out

        //! Brush smoothing spacing - the number of pixels to skip between each pixel used in the smoothing calculations.
        //! This is a way to use a larger area of the image for smoothing without needing to query more pixels and hurt performance.
        size_t m_smoothingSpacing = MinSmoothingSpacing;

        //! These only exist for alternate editing controls. They get stored back into m_brushColor.

        //! Brush stroke intensity percent (0=black, 100=white)
        //! This is displayed instead of color when in monochrome mode.
        float m_intensityPercent = 100.0f;
        //! Brush stroke opacity percent (0=transparent brush stroke, 100=opaque brush stroke)
        float m_opacityPercent = 100.0f;
    };

} // namespace AzFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::PaintBrushBlendMode, "{8C52DEAF-C45B-4C3B-8300-5DBC44CE30AF}");
    AZ_TYPE_INFO_SPECIALIZE(AzFramework::PaintBrushSmoothMode, "{7FEF32F1-54B8-419C-A11E-1CE821BEDF1D}");
} // namespace AZ
