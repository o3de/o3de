/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettings.h>
#include <AzToolsFramework/PaintBrushSettings/PaintBrushSettingsNotificationBus.h>

namespace AzToolsFramework
{
    void PaintBrushSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PaintBrushSettings>()
                ->Version(1)
                ->Field("Radius", &PaintBrushSettings::m_radius)
                ->Field("Intensity", &PaintBrushSettings::m_intensity)
                ->Field("Opacity", &PaintBrushSettings::m_opacity);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PaintBrushSettings>("Paint Brush", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_radius, "Radius", "Radius of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnRadiusChange)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_intensity, "Intensity", "Intensity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnIntensityChange)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_opacity, "Opacity", "Opacity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnOpacityChange);
            }
        }
    }

    void PaintBrushSettings::SetRadius(float radius)
    {
        m_radius = radius;
        OnRadiusChange();
    }

    void PaintBrushSettings::SetIntensity(float intensity)
    {
        m_intensity = intensity;
        OnIntensityChange();
    }

    void PaintBrushSettings::SetOpacity(float opacity)
    {
        m_opacity = opacity;
        OnOpacityChange();
    }

    AZ::u32 PaintBrushSettings::OnIntensityChange()
    {
        // Notify listeners that the configuration changed. This is used to synchronize the paint brush settings with the manipulator.
        PaintBrushSettingsNotificationBus::Broadcast(&PaintBrushSettingsNotificationBus::Events::OnIntensityChanged, m_intensity);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::u32 PaintBrushSettings::OnOpacityChange()
    {
        // Notify listeners that the configuration changed. This is used to synchronize the paint brush settings with the manipulator.
        PaintBrushSettingsNotificationBus::Broadcast(&PaintBrushSettingsNotificationBus::Events::OnOpacityChanged, m_opacity);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::u32 PaintBrushSettings::OnRadiusChange()
    {
        // Notify listeners that the configuration changed. This is used to synchronize the paint brush settings with the manipulator.
        PaintBrushSettingsNotificationBus::Broadcast(&PaintBrushSettingsNotificationBus::Events::OnRadiusChanged, m_radius);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

} // namespace AzToolsFramework
