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
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsRequestBus.h>

namespace AzToolsFramework
{
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
        PaintBrushBlendMode GetBlendMode() const override;
        void SetRadius(float radius) override;
        void SetIntensity(float intensity) override;
        void SetOpacity(float opacity) override;
        void SetBlendMode(PaintBrushBlendMode blendMode) override;

        PaintBrushSettings m_settings;
    };
}; // namespace AzToolsFramework
