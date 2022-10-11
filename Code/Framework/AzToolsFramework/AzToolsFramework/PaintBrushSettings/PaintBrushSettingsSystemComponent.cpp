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

    PaintBrushSettings* PaintBrushSettingsSystemComponent::GetSettings(void)
    {
        return &m_settings;
    }

    float PaintBrushSettingsSystemComponent::GetRadius() const
    {
        return m_settings.GetRadius();
    }

    float PaintBrushSettingsSystemComponent::GetIntensity() const
    {
        return m_settings.GetIntensity();
    }

    float PaintBrushSettingsSystemComponent::GetOpacity() const
    {
        return m_settings.GetOpacity();
    }

    PaintBrushBlendMode PaintBrushSettingsSystemComponent::GetBlendMode() const
    {
        return m_settings.GetBlendMode();
    }

    void PaintBrushSettingsSystemComponent::SetRadius(float radius)
    {
        m_settings.SetRadius(radius);
    }

    void PaintBrushSettingsSystemComponent::SetIntensity(float intensity)
    {
        m_settings.SetIntensity(intensity);
    }

    void PaintBrushSettingsSystemComponent::SetOpacity(float opacity)
    {
        m_settings.SetOpacity(opacity);
    }

    void PaintBrushSettingsSystemComponent::SetBlendMode(PaintBrushBlendMode blendMode)
    {
        m_settings.SetBlendMode(blendMode);
    }
} // namespace AzToolsFramework
