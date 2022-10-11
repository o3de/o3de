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

        float GetRadius() const
        {
            return m_radius;
        }
        float GetIntensity() const
        {
            return m_intensity;
        }
        float GetOpacity() const
        {
            return m_opacity;
        }

        PaintBrushBlendMode GetBlendMode() const
        {
            return m_blendMode;
        }

        void SetRadius(float radius);
        void SetIntensity(float intensity);
        void SetOpacity(float opacity);
        void SetBlendMode(PaintBrushBlendMode blendMode);

    protected:
        //! Paintbrush radius
        float m_radius = 5.0f;
        //! Paintbrush intensity (black to white)
        float m_intensity = 1.0f;
        //! Paintbrush opacity (transparent to opaque)
        float m_opacity = 0.5f;
        //! Paintbrush blend mode
        PaintBrushBlendMode m_blendMode = PaintBrushBlendMode::Normal;

        AZ::u32 OnIntensityChange();
        AZ::u32 OnOpacityChange();
        AZ::u32 OnRadiusChange();
        AZ::u32 OnBlendModeChange();
    };

}; // namespace AzToolsFramework
