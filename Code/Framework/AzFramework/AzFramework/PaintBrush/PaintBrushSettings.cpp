/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/PaintBrush/PaintBrushSettings.h>

namespace AzFramework
{
    void PaintBrushSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PaintBrushSettings>()
                ->Version(7)
                ->Field("Size", &PaintBrushSettings::m_size)
                ->Field("SmoothMode", &PaintBrushSettings::m_smoothMode)
                ->Field("SmoothingRadius", &PaintBrushSettings::m_smoothingRadius)
                ->Field("Color", &PaintBrushSettings::m_brushColor)
                ->Field("Intensity", &PaintBrushSettings::m_intensityPercent)
                ->Field("Opacity", &PaintBrushSettings::m_opacityPercent)
                ->Field("Hardness", &PaintBrushSettings::m_hardnessPercent)
                ->Field("Flow", &PaintBrushSettings::m_flowPercent)
                ->Field("DistancePercent", &PaintBrushSettings::m_distancePercent)
                ->Field("BlendMode", &PaintBrushSettings::m_blendMode)
                ;

            // The EditContext for this class is reflected in AzToolsFramework::GlobalPaintBrushSettings instead of here.
            // This is because the Color field uses a Color Picker widget for editing, which isn't defined in AzFramework.

        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PaintBrushSettings>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "PaintBrush")
                ->Property("size", BehaviorValueProperty(&PaintBrushSettings::m_size))
                ->Property("color", BehaviorValueProperty(&PaintBrushSettings::m_brushColor))
                ->Property("hardnessPercent", BehaviorValueProperty(&PaintBrushSettings::m_hardnessPercent))
                ->Property("flowPercent", BehaviorValueProperty(&PaintBrushSettings::m_flowPercent))
                ->Property("distancePercent", BehaviorValueProperty(&PaintBrushSettings::m_distancePercent))
                ->Property("blendMode", BehaviorValueProperty(&PaintBrushSettings::m_blendMode))
                ->Property("smoothMode", BehaviorValueProperty(&PaintBrushSettings::m_smoothMode))
                ->Property("smoothingRadius", BehaviorValueProperty(&PaintBrushSettings::m_smoothingRadius))
                ;
        }
    }

    // The following settings aren't visible in Eyedropper mode, but are available in Paint / Smooth modes

    bool PaintBrushSettings::GetSizeVisibility() const
    {
        return true;
    }

    bool PaintBrushSettings::GetHardnessVisibility() const
    {
        return true;
    }

    bool PaintBrushSettings::GetFlowVisibility() const
    {
        return true;
    }

    bool PaintBrushSettings::GetDistanceVisibility() const
    {
        return true;
    }

    bool PaintBrushSettings::GetBlendModeVisibility() const
    {
        return true;
    }

    // The following settings are only visible in Smooth mode

    bool PaintBrushSettings::GetSmoothingRadiusVisibility() const
    {
        return true;
    }

    bool PaintBrushSettings::GetSmoothModeVisibility() const
    {
        return true;
    }

    // The color / intensity settings have their visibility controlled by both the color mode and the brush mode.

    bool PaintBrushSettings::GetColorVisibility() const
    {
        return true;
    }

    bool PaintBrushSettings::GetIntensityVisibility() const
    {
        return true;
    }

    // Opacity is always visible, regardless of brush mode or color mode.

    bool PaintBrushSettings::GetOpacityVisibility() const
    {
        return true;
    }

    // Make the brush size ranges configurable so that it can be appropriate for whatever type of data is being painted.

    float PaintBrushSettings::GetSizeMin() const
    {
        return m_sizeMin;
    }

    float PaintBrushSettings::GetSizeMax() const
    {
        return m_sizeMax;
    }

    float PaintBrushSettings::GetSizeStep() const
    {
        // Set the step size to give us 100 values across the range.
        // This is an arbitrary choice, but it seems like a good number of step sizes for a slider control.
        return (GetSizeMax() - GetSizeMin()) / 100.0f;
    }

    void PaintBrushSettings::SetSizeRange(float minSize, float maxSize)
    {
        // Make sure the min and max sizes are valid ranges
        m_sizeMax = AZStd::max(maxSize, 0.0f);
        m_sizeMin = AZStd::clamp(minSize, 0.0f, m_sizeMax);

        // Clamp our current paintbrush size to fall within the new min/max ranges.
        m_size = AZStd::clamp(m_size, m_sizeMin, m_sizeMax);

        OnSizeRangeChanged();
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetBlendMode(PaintBrushBlendMode blendMode)
    {
        m_blendMode = blendMode;
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetSmoothMode(PaintBrushSmoothMode smoothMode)
    {
        m_smoothMode = smoothMode;
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetColor(const AZ::Color& color)
    {
        m_brushColor = color;

        // Keep our editable intensity / opacity values in sync with the brush color.
        m_intensityPercent = m_brushColor.GetR() * 100.0f;
        m_opacityPercent = m_brushColor.GetA() * 100.0f;

        OnSettingsChanged();
    }

    void PaintBrushSettings::SetSize(float size)
    {
        m_size = AZStd::clamp(size, m_sizeMin, m_sizeMax);
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

    void PaintBrushSettings::SetSmoothingRadius(size_t smoothingRadius)
    {
        m_smoothingRadius = AZStd::clamp(smoothingRadius, MinSmoothingRadius, MaxSmoothingRadius);
        OnSettingsChanged();
    }

    void PaintBrushSettings::SetSmoothingSpacing(size_t smoothingSpacing)
    {
        m_smoothingSpacing = AZStd::clamp(smoothingSpacing, MinSmoothingSpacing, MaxSmoothingSpacing);
        OnSettingsChanged();
    }

    AZ::u32 PaintBrushSettings::OnColorChanged()
    {
        // Keep our editable intensity and opacity in sync with the brush color.
        m_intensityPercent = m_brushColor.GetR() * 100.0f;
        m_opacityPercent = m_brushColor.GetA() * 100.0f;

        return OnSettingsChanged();
    }

    AZ::u32 PaintBrushSettings::OnIntensityChanged()
    {
        const float intensity = m_intensityPercent / 100.0f;
        m_brushColor = AZ::Color(intensity, intensity, intensity, m_brushColor.GetA());
        return OnSettingsChanged();
    }

    AZ::u32 PaintBrushSettings::OnOpacityChanged()
    {
        m_brushColor.SetA(m_opacityPercent / 100.0f);
        return OnSettingsChanged();
    }

    // Notification functions for editing changes that aren't used for anything in PaintBrushSettings but can be overridden.

    void PaintBrushSettings::OnSizeRangeChanged()
    {
    }

    AZ::u32 PaintBrushSettings::OnSettingsChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }


} // namespace AzFramework
