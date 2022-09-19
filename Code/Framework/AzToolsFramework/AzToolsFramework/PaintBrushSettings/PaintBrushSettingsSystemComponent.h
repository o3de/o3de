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
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsRequestBus.h>

namespace AzToolsFramework
{
    //! PaintBrushSettings defines the specific global paintbrush settings.
    //! They can be modified directly through the Property Editor or indirectly via the PaintBrushSettingsRequestBus.
    class PaintBrushSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrushSettings, AZ::SystemAllocator, 0);
        AZ_RTTI(PaintBrushSettings, "{CE5EFFE2-14E5-4A9F-9B0F-695F66744A50}");
        static void Reflect(AZ::ReflectContext* context);

        virtual ~PaintBrushSettings() = default;

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

        void SetRadius(float radius);
        void SetIntensity(float intensity);
        void SetOpacity(float opacity);

    protected:
        //! Paintbrush radius
        float m_radius = 5.0f;
        //! Paintbrush intensity (black to white)
        float m_intensity = 1.0f;
        //! Paintbrush opacity (transparent to opaque)
        float m_opacity = 0.5f;

        AZ::u32 OnIntensityChange();
        AZ::u32 OnOpacityChange();
        AZ::u32 OnRadiusChange();
    };

    //! PaintBrushSettingsSystemComponent owns the current global paintbrush settings for the Editor.
    class PaintBrushSettingsSystemComponent
        : public AZ::Component
        , private AzToolsFramework::PaintBrushSettingsRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PaintBrushSettingsSystemComponent, "{78C4CCDE-BDDE-4A49-9534-6D0B62404338}");

        PaintBrushSettingsSystemComponent() = default;
        ~PaintBrushSettingsSystemComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        void Activate() override;
        void Deactivate() override;

        // PaintBrushSettingsRequestBus overrides...
        PaintBrushSettings* GetSettings() override;
        float GetRadius() const override;
        float GetIntensity() const override;
        float GetOpacity() const override;
        void SetRadius(float radius) override;
        void SetIntensity(float intensity) override;
        void SetOpacity(float opacity) override;

        PaintBrushSettings m_settings;
    };
}; // namespace AzToolsFramework
