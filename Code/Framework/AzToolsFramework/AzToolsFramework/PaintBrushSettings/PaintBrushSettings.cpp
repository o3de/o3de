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
                ->Field("Intensity", &PaintBrushSettings::m_intensityPercent)
                ->Field("Opacity", &PaintBrushSettings::m_opacityPercent)
                ->Field("Hardness", &PaintBrushSettings::m_hardnessPercent)
                ->Field("Flow", &PaintBrushSettings::m_flowPercent)
                ->Field("DistancePercent", &PaintBrushSettings::m_distancePercent)
                ->Field("BlendMode", &PaintBrushSettings::m_blendMode)
                ;


            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PaintBrushSettings>("Paint Brush", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_size, "Size",
                        "Size/diameter of the brush stamp in meters.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 2)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_intensityPercent, "Intensity",
                        "Intensity/color percent of the paint brush. 0% = black, 100% = white.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_opacityPercent, "Opacity",
                        "Opacity percent of each paint brush stroke. 0% = transparent, 100% = opaque.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_hardnessPercent, "Hardness",
                        "Falloff percent around the edges of each paint brush stamp. 0% = soft falloff, 100% = hard edges.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_flowPercent, "Flow",
                        "The opacity percent of each paint brush stamp. 0% = transparent, 100% = opaque.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_distancePercent, "Distance",
                        "Brush distance to move between stamps in % of brush size. 1% = high overlap, 100% = non-overlapping stamps.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 300.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                    ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &PaintBrushSettings::m_blendMode, "Mode", "Blend mode of the brush stroke.")
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

    void PaintBrushSettings::SetIntensityPercent(float intensityPercent)
    {
        m_intensityPercent = AZStd::clamp(intensityPercent, 0.0f, 100.0f);
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetOpacityPercent(float opacityPercent)
    {
        m_opacityPercent = AZStd::clamp(opacityPercent, 0.0f, 100.0f);
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetBlendMode(PaintBrushBlendMode blendMode)
    {
        m_blendMode = blendMode;
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetSize(float size)
    {
        m_size = AZStd::max(size, 0.0f);
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetHardnessPercent(float hardnessPercent)
    {
        m_hardnessPercent = AZStd::clamp(hardnessPercent, 0.0f, 100.0f);
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetFlowPercent(float flowPercent)
    {
        m_flowPercent = AZStd::clamp(flowPercent, 0.0f, 100.0f);
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetDistancePercent(float distancePercent)
    {
        // Distance percent is *normally* 0-100%, but values above 100% are reasonable as well, so we don't clamp the upper limit.
        m_distancePercent = AZStd::max(distancePercent, 0.0f);
        OnSettingsChanged();
    }

    AZ::u32 PaintBrushSettings::OnSettingsChanged()
    {
        // Notify listeners that the configuration changed. This is used to refresh the paint settings window.
        PaintBrushSettingsNotificationBus::Broadcast(&PaintBrushSettingsNotificationBus::Events::OnSettingsChanged, *this);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }


} // namespace AzToolsFramework
