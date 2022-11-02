/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsSystemComponent.h>

namespace AzToolsFramework
{
    void PaintBrushSettingsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        PaintBrushSettings::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PaintBrushSettingsSystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("Settings", &PaintBrushSettingsSystemComponent::m_settings)
                ;
        }
    }

    void PaintBrushSettingsSystemComponent::Activate()
    {
        PaintBrushSettingsRequestBus::Handler::BusConnect();
    }

    void PaintBrushSettingsSystemComponent::Deactivate()
    {
        PaintBrushSettingsRequestBus::Handler::BusDisconnect();
    }

    PaintBrushSettings* PaintBrushSettingsSystemComponent::GetSettingsPointerForPropertyEditor()
    {
        return &m_settings;
    }

    PaintBrushSettings PaintBrushSettingsSystemComponent::GetSettings() const
    {
        return m_settings;
    }

    PaintBrushColorMode PaintBrushSettingsSystemComponent::GetBrushColorMode() const
    {
        return m_settings.GetColorMode();
    }

    void PaintBrushSettingsSystemComponent::SetBrushColorMode(PaintBrushColorMode colorMode)
    {
        m_settings.SetColorMode(colorMode);
    }

    float PaintBrushSettingsSystemComponent::GetSize() const
    {
        return m_settings.GetSize();
    }

    AZ::Color PaintBrushSettingsSystemComponent::GetColor() const
    {
        return m_settings.GetColor();
    }

    float PaintBrushSettingsSystemComponent::GetHardnessPercent() const
    {
        return m_settings.GetHardnessPercent();
    }

    float PaintBrushSettingsSystemComponent::GetFlowPercent() const
    {
        return m_settings.GetFlowPercent();
    }

    float PaintBrushSettingsSystemComponent::GetDistancePercent() const
    {
        return m_settings.GetDistancePercent();
    }

    PaintBrushBlendMode PaintBrushSettingsSystemComponent::GetBlendMode() const
    {
        return m_settings.GetBlendMode();
    }

    void PaintBrushSettingsSystemComponent::SetSize(float size)
    {
        m_settings.SetSize(size);
    }

    void PaintBrushSettingsSystemComponent::SetColor(const AZ::Color& color)
    {
        m_settings.SetColor(color);
    }

    void PaintBrushSettingsSystemComponent::SetHardnessPercent(float hardnessPercent)
    {
        m_settings.SetHardnessPercent(hardnessPercent);
    }

    void PaintBrushSettingsSystemComponent::SetFlowPercent(float flowPercent)
    {
        m_settings.SetFlowPercent(flowPercent);
    }

    void PaintBrushSettingsSystemComponent::SetDistancePercent(float distancePercent)
    {
        m_settings.SetDistancePercent(distancePercent);
    }

    void PaintBrushSettingsSystemComponent::SetBlendMode(PaintBrushBlendMode blendMode)
    {
        m_settings.SetBlendMode(blendMode);
    }
} // namespace AzToolsFramework
