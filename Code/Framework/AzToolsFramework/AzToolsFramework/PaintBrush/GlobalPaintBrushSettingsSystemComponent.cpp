/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettings.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsSystemComponent.h>

namespace AzToolsFramework
{
    void GlobalPaintBrushSettingsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        GlobalPaintBrushSettings::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GlobalPaintBrushSettingsSystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("Settings", &GlobalPaintBrushSettingsSystemComponent::m_settings)
                ;
        }
    }

    void GlobalPaintBrushSettingsSystemComponent::Activate()
    {
        GlobalPaintBrushSettingsRequestBus::Handler::BusConnect();
        ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void GlobalPaintBrushSettingsSystemComponent::Deactivate()
    {
        ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        GlobalPaintBrushSettingsRequestBus::Handler::BusDisconnect();
    }

    void GlobalPaintBrushSettingsSystemComponent::OnActionRegistrationHook()
    {
        PaintBrushManipulator::RegisterActions();
    }

    void GlobalPaintBrushSettingsSystemComponent::OnMenuBindingHook()
    {
        PaintBrushManipulator::BindActionsToMenus();
    }

    GlobalPaintBrushSettings* GlobalPaintBrushSettingsSystemComponent::GetSettingsPointerForPropertyEditor()
    {
        return &m_settings;
    }

    GlobalPaintBrushSettings GlobalPaintBrushSettingsSystemComponent::GetSettings() const
    {
        return m_settings;
    }

    PaintBrushMode GlobalPaintBrushSettingsSystemComponent::GetBrushMode() const
    {
        return m_settings.GetBrushMode();
    }

    void GlobalPaintBrushSettingsSystemComponent::SetBrushMode(PaintBrushMode brushMode)
    {
        m_settings.SetBrushMode(brushMode);
    }

    PaintBrushColorMode GlobalPaintBrushSettingsSystemComponent::GetBrushColorMode() const
    {
        return m_settings.GetColorMode();
    }

    void GlobalPaintBrushSettingsSystemComponent::SetBrushColorMode(PaintBrushColorMode colorMode)
    {
        m_settings.SetColorMode(colorMode);
    }

    float GlobalPaintBrushSettingsSystemComponent::GetSize() const
    {
        return m_settings.GetSize();
    }

    AZStd::pair<float, float> GlobalPaintBrushSettingsSystemComponent::GetSizeRange() const
    {
        return m_settings.GetSizeRange();
    }

    AZ::Color GlobalPaintBrushSettingsSystemComponent::GetColor() const
    {
        return m_settings.GetColor();
    }

    float GlobalPaintBrushSettingsSystemComponent::GetHardnessPercent() const
    {
        return m_settings.GetHardnessPercent();
    }

    float GlobalPaintBrushSettingsSystemComponent::GetFlowPercent() const
    {
        return m_settings.GetFlowPercent();
    }

    float GlobalPaintBrushSettingsSystemComponent::GetDistancePercent() const
    {
        return m_settings.GetDistancePercent();
    }

    AzFramework::PaintBrushBlendMode GlobalPaintBrushSettingsSystemComponent::GetBlendMode() const
    {
        return m_settings.GetBlendMode();
    }

    AzFramework::PaintBrushSmoothMode GlobalPaintBrushSettingsSystemComponent::GetSmoothMode() const
    {
        return m_settings.GetSmoothMode();
    }

    size_t GlobalPaintBrushSettingsSystemComponent::GetSmoothingRadius() const
    {
        return m_settings.GetSmoothingRadius();
    }

    size_t GlobalPaintBrushSettingsSystemComponent::GetSmoothingSpacing() const
    {
        return m_settings.GetSmoothingSpacing();
    }

    void GlobalPaintBrushSettingsSystemComponent::SetSize(float size)
    {
        m_settings.SetSize(size);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetSizeRange(float minSize, float maxSize)
    {
        m_settings.SetSizeRange(minSize, maxSize);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetColor(const AZ::Color& color)
    {
        m_settings.SetColor(color);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetHardnessPercent(float hardnessPercent)
    {
        m_settings.SetHardnessPercent(hardnessPercent);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetFlowPercent(float flowPercent)
    {
        m_settings.SetFlowPercent(flowPercent);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetDistancePercent(float distancePercent)
    {
        m_settings.SetDistancePercent(distancePercent);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetBlendMode(AzFramework::PaintBrushBlendMode blendMode)
    {
        m_settings.SetBlendMode(blendMode);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetSmoothMode(AzFramework::PaintBrushSmoothMode smoothMode)
    {
        m_settings.SetSmoothMode(smoothMode);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetSmoothingRadius(size_t radius)
    {
        m_settings.SetSmoothingRadius(radius);
    }

    void GlobalPaintBrushSettingsSystemComponent::SetSmoothingSpacing(size_t spacing)
    {
        m_settings.SetSmoothingSpacing(spacing);
    }

} // namespace AzToolsFramework
