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
        PaintBrushSettings* GetSettingsPointerForPropertyEditor() override;
        PaintBrushSettings GetSettings() const override;
        PaintBrushMode GetBrushMode() const override;
        void SetBrushMode(PaintBrushMode brushMode) override;
        PaintBrushColorMode GetBrushColorMode() const override;
        void SetBrushColorMode(PaintBrushColorMode colorMode) override;
        float GetSize() const override;
        AZStd::pair<float, float> GetSizeRange() const override;
        AZ::Color GetColor() const override;
        float GetHardnessPercent() const override;
        float GetFlowPercent() const override;
        float GetDistancePercent() const override;
        PaintBrushBlendMode GetBlendMode() const override;
        PaintBrushSmoothMode GetSmoothMode() const override;
        void SetSize(float size) override;
        void SetSizeRange(float minSize, float maxSize) override;
        void SetColor(const AZ::Color& color) override;
        void SetHardnessPercent(float hardnessPercent) override;
        void SetFlowPercent(float flowPercent) override;
        void SetDistancePercent(float distancePercent) override;
        void SetBlendMode(PaintBrushBlendMode blendMode) override;
        void SetSmoothMode(PaintBrushSmoothMode smoothMode) override;

        PaintBrushSettings m_settings;
    };
}; // namespace AzToolsFramework
