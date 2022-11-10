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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyColorCtrl.hxx>

namespace AzToolsFramework
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

    //! The different types of functionality offered by the paint brush tool
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(PaintBrushMode, uint8_t,
        (Paintbrush, 0),    //!< Uses color, opacity, and other brush settings to 'paint' values for an abstract data source
        (Eyedropper, 1),    //!< Gets the current value underneath the brush from an abstract data source
        (Smooth, 2)         //!< Smooths/blurs the existing values in an abstract data source
    );

    //! The different types of blend modes supported by the paint brush tool.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(PaintBrushColorMode, uint8_t,
        (Greyscale, 0), 
        (SRGB, 1), 
        (LinearColor, 2) 
    );

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

    //! Defines the specific global paintbrush settings.
    //! They can be modified directly through the Property Editor or indirectly via the PaintBrushSettingsRequestBus.
    class PaintBrushSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrushSettings, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PaintBrushSettings, "{CE5EFFE2-14E5-4A9F-9B0F-695F66744A50}");
        static void Reflect(AZ::ReflectContext* context);

        PaintBrushSettings() = default;
        ~PaintBrushSettings() = default;

        // Overall paintbrush settings

        PaintBrushMode GetBrushMode() const
        {
            return m_brushMode;
        }

        void SetBrushMode(PaintBrushMode brushMode);

        PaintBrushColorMode GetColorMode() const
        {
            return m_colorMode;
        }

        void SetColorMode(PaintBrushColorMode colorMode);

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

    protected:
        AzToolsFramework::ColorEditorConfiguration GetColorEditorConfig();

        AZ::u32 OnColorChanged();
        AZ::u32 OnIntensityChanged();
        AZ::u32 OnOpacityChanged();

        bool GetColorReadOnly() const;
        bool GetIntensityReadOnly() const;
        bool GetOpacityReadOnly() const;

        bool GetSizeVisibility() const;
        bool GetColorVisibility() const;
        bool GetIntensityVisibility() const;
        bool GetOpacityVisibility() const;
        bool GetHardnessVisibility() const;
        bool GetFlowVisibility() const;
        bool GetDistanceVisibility() const;
        bool GetBlendModeVisibility() const;
        bool GetSmoothModeVisibility() const;

        float GetSizeMin() const;
        float GetSizeMax() const;
        float GetSizeStep() const;

        //! Brush settings brush mode
        PaintBrushMode m_brushMode = PaintBrushMode::Paintbrush;

        //! Brush settings color mode
        PaintBrushColorMode m_colorMode = PaintBrushColorMode::Greyscale;

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

        //! These only exist for alternate editing controls. They get stored back into m_brushColor.

        //! Brush stroke intensity percent (0=black, 100=white)
        //! This is displayed instead of color when in monochrome mode.
        float m_intensityPercent = 100.0f;
        //! Brush stroke opacity percent (0=transparent brush stroke, 100=opaque brush stroke)
        float m_opacityPercent = 100.0f;

        AZ::u32 OnSettingsChanged();
    };

} // namespace AzToolsFramework

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::PaintBrushMode, "{88C6AEA1-5424-4F3A-9E22-6D55C050F06C}");
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::PaintBrushColorMode, "{0D3B0981-BFB3-47E0-9E28-99CFB540D5AC}");
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::PaintBrushBlendMode, "{8C52DEAF-C45B-4C3B-8300-5DBC44CE30AF}");
    AZ_TYPE_INFO_SPECIALIZE(AzToolsFramework::PaintBrushSmoothMode, "{7FEF32F1-54B8-419C-A11E-1CE821BEDF1D}");
} // namespace AZ
