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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettings.h>

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
       - Intensity: The color of the paint on the paintbrush. We only support monochromatic painting, so this is just a greyscale
         intensity from black to white. This color is fully applied to each brush stroke.
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
    enum class PaintBrushBlendMode : uint8_t
    {
        Normal,  //!< Alpha blends between the paint brush value and the existing value 
        Add,        //!< Adds the paint brush value to the existing value
        Subtract,   //!< Subtracts the paint brush value from the existing value
        Multiply,   //!< Multiplies the paint brush value with the existing value. (Darkens)
        Screen,     //!< Inverts the two values, multiplies, then inverts again. (Lightens)
        Darken,     //!< Keeps the minimum of the paint brush value and the existing value
        Lighten,    //!< Keeps the maximum of the paint brush value and the existing value
        Average,    //!< Takes the average of the paint brush value and the existing value
        Overlay     //!< Combines Multiply and Screen - darkens when brush < 0.5, lightens when brush > 0.5
    };

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

        // Stroke settings

        float GetIntensityPercent() const
        {
            return m_intensityPercent;
        }
        float GetOpacityPercent() const
        {
            return m_opacityPercent;
        }

        PaintBrushBlendMode GetBlendMode() const
        {
            return m_blendMode;
        }

        void SetIntensityPercent(float intensityPercent);
        void SetOpacityPercent(float opacityPercent);
        void SetBlendMode(PaintBrushBlendMode blendMode);

        // Stamp settings

        float GetSize() const
        {
            return m_size;
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
        void SetHardnessPercent(float hardnessPercent);
        void SetFlowPercent(float flowPercent);
        void SetDistancePercent(float distancePercent);

    protected:
        //! Brush stroke intensity percent (0=black, 100=white)
        float m_intensityPercent = 100.0f;
        //! Brush stroke opacity percent (0=transparent brush stroke, 100=opaque brush stroke)
        float m_opacityPercent = 100.0f;
        //! Brush stroke blend mode
        PaintBrushBlendMode m_blendMode = PaintBrushBlendMode::Normal;

        //! Brush stamp diameter in meters
        float m_size = 10.0f;
        //! Brush stamp hardness percent (0=soft falloff, 100=hard edge)
        float m_hardnessPercent = 100.0f;
        //! Brush stamp flow percent (0=transparent stamps, 100=opaque stamps)
        float m_flowPercent = 100.0f;
        //! Brush distance to move between stamps in % of paintbrush size. (25% is the default in Photoshop.)
        float m_distancePercent = 25.0f;

        AZ::u32 OnSettingsChanged();
    };

}; // namespace AzToolsFramework
