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
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettings.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsRequestBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    //! GlobalPaintBrushSettingsSystemComponent owns the current global paintbrush settings for the Editor.
    class GlobalPaintBrushSettingsSystemComponent
        : public AZ::Component
        , public AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
        , private AzToolsFramework::GlobalPaintBrushSettingsRequestBus::Handler
    {
    public:
        AZ_COMPONENT(GlobalPaintBrushSettingsSystemComponent, "{78C4CCDE-BDDE-4A49-9534-6D0B62404338}");

        GlobalPaintBrushSettingsSystemComponent() = default;
        ~GlobalPaintBrushSettingsSystemComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;

    protected:
        void Activate() override;
        void Deactivate() override;

        // GlobalPaintBrushSettingsRequestBus overrides...
        GlobalPaintBrushSettings* GetSettingsPointerForPropertyEditor() override;
        GlobalPaintBrushSettings GetSettings() const override;
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
        AzFramework::PaintBrushBlendMode GetBlendMode() const override;
        AzFramework::PaintBrushSmoothMode GetSmoothMode() const override;
        size_t GetSmoothingRadius() const override;
        size_t GetSmoothingSpacing() const override;
        void SetSize(float size) override;
        void SetSizeRange(float minSize, float maxSize) override;
        void SetColor(const AZ::Color& color) override;
        void SetHardnessPercent(float hardnessPercent) override;
        void SetFlowPercent(float flowPercent) override;
        void SetDistancePercent(float distancePercent) override;
        void SetBlendMode(AzFramework::PaintBrushBlendMode blendMode) override;
        void SetSmoothMode(AzFramework::PaintBrushSmoothMode smoothMode) override;
        void SetSmoothingRadius(size_t radius) override;
        void SetSmoothingSpacing(size_t spacing) override;

        GlobalPaintBrushSettings m_settings;
    };
}; // namespace AzToolsFramework
