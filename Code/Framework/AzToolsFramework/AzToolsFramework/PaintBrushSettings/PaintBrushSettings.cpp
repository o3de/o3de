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
                ->Version(3)
                ->Field("Size", &PaintBrushSettings::m_size)
                ->Field("Intensity", &PaintBrushSettings::m_intensity)
                ->Field("Opacity", &PaintBrushSettings::m_opacity)
                ->Field("Hardness", &PaintBrushSettings::m_hardness)
                ->Field("BlendMode", &PaintBrushSettings::m_blendMode);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PaintBrushSettings>("Paint Brush", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_size, "Size", "Size/diameter of the paint brush (m).")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_intensity, "Intensity", "Intensity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_opacity, "Opacity", "Opacity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_hardness, "Hardness",
                        "Falloff around the edges of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.025f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &PaintBrushSettings::m_blendMode, "Mode", "Blend mode of the paint brush.")
                    ->EnumAttribute(PaintBrushBlendMode::Normal, "Normal")
                    ->EnumAttribute(PaintBrushBlendMode::Multiply, "Multiply")
                    ->EnumAttribute(PaintBrushBlendMode::Screen, "Screen")
                    ->EnumAttribute(PaintBrushBlendMode::Add, "Linear Dodge (Add)")
                    ->EnumAttribute(PaintBrushBlendMode::Subtract, "Subtract")
                    ->EnumAttribute(PaintBrushBlendMode::Darken, "Darken (Min)")
                    ->EnumAttribute(PaintBrushBlendMode::Lighten, "Lighten (Max)")
                    ->EnumAttribute(PaintBrushBlendMode::Average, "Average")
                    ->EnumAttribute(PaintBrushBlendMode::Overlay, "Overlay")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ;
            }
        }
    }

    void PaintBrushSettings::SetSize(float size)
    {
        m_size = size;
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetIntensity(float intensity)
    {
        m_intensity = intensity;
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetOpacity(float opacity)
    {
        m_opacity = opacity;
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetHardness(float hardness)
    {
        m_hardness = hardness;
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetBlendMode(PaintBrushBlendMode blendMode)
    {
        m_blendMode = blendMode;
        OnSettingsChanged();
    }

    AZ::u32 PaintBrushSettings::OnSettingsChanged()
    {
        // Notify listeners that the configuration changed. This is used to refresh the paint settings window.
        PaintBrushSettingsNotificationBus::Broadcast(&PaintBrushSettingsNotificationBus::Events::OnSettingsChanged, *this);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }


} // namespace AzToolsFramework
