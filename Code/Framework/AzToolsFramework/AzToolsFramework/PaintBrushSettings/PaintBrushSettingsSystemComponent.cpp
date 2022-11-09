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

    PaintBrushMode PaintBrushSettingsSystemComponent::GetBrushMode() const
    {
        return m_settings.GetBrushMode();
    }

    void PaintBrushSettingsSystemComponent::SetBrushMode(PaintBrushMode brushMode)
    {
        m_settings.SetBrushMode(brushMode);
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

    AZStd::pair<float, float> PaintBrushSettingsSystemComponent::GetSizeRange() const
    {
        return m_settings.GetSizeRange();
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

    PaintBrushSmoothMode PaintBrushSettingsSystemComponent::GetSmoothMode() const
    {
        return m_settings.GetSmoothMode();
    }

    void PaintBrushSettingsSystemComponent::SetSize(float size)
    {
        m_settings.SetSize(size);
    }

    void PaintBrushSettingsSystemComponent::SetSizeRange(float minSize, float maxSize)
    {
        m_settings.SetSizeRange(minSize, maxSize);
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

    void PaintBrushSettingsSystemComponent::SetSmoothMode(PaintBrushSmoothMode smoothMode)
    {
        m_settings.SetSmoothMode(smoothMode);
    }
} // namespace AzToolsFramework
