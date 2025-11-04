/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettings.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsNotificationBus.h>
#include <AzToolsFramework/PaintBrush/GlobalPaintBrushSettingsRequestBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyColorCtrl.hxx>

namespace AzToolsFramework
{
    void GlobalPaintBrushSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GlobalPaintBrushSettings, PaintBrushSettings>()
                ->Version(2)
                ->Field("BrushMode", &GlobalPaintBrushSettings::m_brushMode)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                // The EditContext for PaintBrushSettings is reflected here instead of in PaintBrushSettings in AzFramework because
                // we want to use the ColorEditorConfiguration to control the Color Picker widget for the Color field.
                // The ColorEditorConfiguration and the Color picker widget are defined at the AzToolsFramework level, so the
                // EditContext needs to be at this level as well to be able to refer to them.
                editContext->Class<PaintBrushSettings>("Paint Brush", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        // There's no special meaning to 100, we just want the PaintBrushSettings to display after the GlobalPaintBrushSettings
                        ->Attribute(AZ::Edit::Attributes::DisplayOrder, 100)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_size, "Size",
                        "Size/diameter of the brush stamp in meters.")
                        ->Attribute(AZ::Edit::Attributes::Min, &PaintBrushSettings::GetSizeMin)
                        ->Attribute(AZ::Edit::Attributes::Max, &PaintBrushSettings::GetSizeMax)
                        ->Attribute(AZ::Edit::Attributes::Step, &PaintBrushSettings::GetSizeStep)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 2)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetSizeVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PaintBrushSettings::m_brushColor, "Color", "Color of the paint brush.")
                    ->Attribute(
                        "ColorEditorConfiguration",
                        []([[maybe_unused]] void* voidHandler) -> AzToolsFramework::ColorEditorConfiguration
                        {
                            enum ColorSpace : uint32_t
                            {
                                LinearSRGB,
                                SRGB
                            };

                            AzToolsFramework::ColorEditorConfiguration configuration;
                            configuration.m_colorPickerDialogConfiguration = AzQtComponents::ColorPicker::Configuration::RGBA;

                            // The property is in linear color space, but we should display the colors using SRGB.
                            configuration.m_propertyColorSpaceId = ColorSpace::LinearSRGB;
                            configuration.m_colorPickerDialogColorSpaceId = ColorSpace::SRGB;
                            configuration.m_colorSwatchColorSpaceId = ColorSpace::SRGB;

                            configuration.m_colorSpaceNames[ColorSpace::LinearSRGB] = "Linear sRGB";
                            configuration.m_colorSpaceNames[ColorSpace::SRGB] = "sRGB";

                            configuration.m_transformColorCallback =
                                [](const AZ::Color& color, uint32_t fromColorSpaceId, uint32_t toColorSpaceId)
                            {
                                if (fromColorSpaceId == toColorSpaceId)
                                {
                                    return color;
                                }
                                else if (fromColorSpaceId == ColorSpace::LinearSRGB && toColorSpaceId == ColorSpace::SRGB)
                                {
                                    return color.LinearToGamma();
                                }
                                else if (fromColorSpaceId == ColorSpace::SRGB && toColorSpaceId == ColorSpace::LinearSRGB)
                                {
                                    return color.GammaToLinear();
                                }
                                else
                                {
                                    AZ_Error("ColorEditorConfiguration", false, "Invalid color space combination");
                                    return color;
                                }
                            };

                            return configuration;
                        })
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetColorVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnColorChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &PaintBrushSettings::m_intensityPercent,
                        "Intensity",
                        "Intensity/color percent of the paint brush. 0% = black, 100% = white.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetIntensityVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnIntensityChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &PaintBrushSettings::m_opacityPercent,
                        "Opacity",
                        "Opacity percent of each paint brush stroke. 0% = transparent, 100% = opaque.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetOpacityVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnOpacityChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider,
                        &PaintBrushSettings::m_hardnessPercent,
                        "Hardness",
                        "Falloff percent around the edges of each paint brush stamp. 0% = soft falloff, 100% = hard edges.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetHardnessVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_flowPercent, "Flow",
                        "The opacity percent of each paint brush stamp. 0% = transparent, 100% = opaque.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.5f)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 1)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetFlowVisibility)
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
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetDistanceVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &PaintBrushSettings::m_blendMode, "Blend Mode", "Blend mode of the brush stroke.")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Normal, "Normal")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Multiply, "Multiply")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Screen, "Screen")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Add, "Linear Dodge (Add)")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Subtract, "Subtract")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Darken, "Darken (Min)")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Lighten, "Lighten (Max)")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Average, "Average")
                        ->EnumAttribute(AzFramework::PaintBrushBlendMode::Overlay, "Overlay")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetBlendModeVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &PaintBrushSettings::m_smoothMode,
                        "Smooth Mode", "Smooth mode of the brush stroke.")
                        ->EnumAttribute(AzFramework::PaintBrushSmoothMode::Gaussian, "Weighted Average (Gaussian)")
                        ->EnumAttribute(AzFramework::PaintBrushSmoothMode::Mean, "Average (Mean)")
                        ->EnumAttribute(AzFramework::PaintBrushSmoothMode::Median, "Middle Value (Median)")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetSmoothModeVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrushSettings::m_smoothingRadius, "Smoothing Radius",
                        "The number of values in each direction to use for smoothing (a radius of 1 = 3x3 smoothing kernel).")
                        ->Attribute(AZ::Edit::Attributes::Min, MinSmoothingRadius)
                        ->Attribute(AZ::Edit::Attributes::Max, MaxSmoothingRadius)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &PaintBrushSettings::GetSmoothingRadiusVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrushSettings::OnSettingsChanged)
                    ;

                editContext->Class<GlobalPaintBrushSettings>("Global Paint Brush Settings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &GlobalPaintBrushSettings::m_brushMode, "Brush Mode", "Brush functionality.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<AzToolsFramework::PaintBrushMode>())
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GlobalPaintBrushSettings::OnBrushModeChanged)
                    ;
            }
        }
    }

    void GlobalPaintBrushSettings::OnSizeRangeChanged()
    {
        // Notify listeners that a change occurred that affects how the properties are displayed.
        GlobalPaintBrushSettingsNotificationBus::Broadcast(&GlobalPaintBrushSettingsNotificationBus::Events::OnVisiblePropertiesChanged);
    }

    void GlobalPaintBrushSettings::OnBrushModeChanged()
    {
        // Notify listeners that the paint brush mode has changed.
        GlobalPaintBrushSettingsNotificationBus::Broadcast(
            &GlobalPaintBrushSettingsNotificationBus::Events::OnPaintBrushModeChanged, m_brushMode);

        // Notify listeners that a change occurred that affects how the properties are displayed.
        GlobalPaintBrushSettingsNotificationBus::Broadcast(&GlobalPaintBrushSettingsNotificationBus::Events::OnVisiblePropertiesChanged);
    }

    void GlobalPaintBrushSettings::OnColorModeChanged()
    {
        // Notify listeners that a change occurred that affects how the properties are displayed.
        GlobalPaintBrushSettingsNotificationBus::Broadcast(&GlobalPaintBrushSettingsNotificationBus::Events::OnVisiblePropertiesChanged);
    }

    AZ::u32 GlobalPaintBrushSettings::OnSettingsChanged()
    {
        // Notify listeners that the configuration changed. This is used to refresh the global paint settings window.
        GlobalPaintBrushSettingsNotificationBus::Broadcast(&GlobalPaintBrushSettingsNotificationBus::Events::OnSettingsChanged, *this);
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

        // The following settings aren't visible in Eyedropper mode, but are available in Paint / Smooth modes

    bool GlobalPaintBrushSettings::GetSizeVisibility() const
    {
        return (m_brushMode != PaintBrushMode::Eyedropper);
    }

    bool GlobalPaintBrushSettings::GetHardnessVisibility() const
    {
        return (m_brushMode != PaintBrushMode::Eyedropper);
    }

    bool GlobalPaintBrushSettings::GetFlowVisibility() const
    {
        return (m_brushMode != PaintBrushMode::Eyedropper);
    }

    bool GlobalPaintBrushSettings::GetDistanceVisibility() const
    {
        return (m_brushMode != PaintBrushMode::Eyedropper);
    }

    bool GlobalPaintBrushSettings::GetBlendModeVisibility() const
    {
        return (m_brushMode != PaintBrushMode::Eyedropper);
    }

    // The following settings are only visible in Smooth mode

    bool GlobalPaintBrushSettings::GetSmoothingRadiusVisibility() const
    {
        return (m_brushMode == PaintBrushMode::Smooth);
    }

    bool GlobalPaintBrushSettings::GetSmoothModeVisibility() const
    {
        return (m_brushMode == PaintBrushMode::Smooth);
    }

    // The color / intensity settings have their visibility controlled by both the color mode and the brush mode.

    bool GlobalPaintBrushSettings::GetColorVisibility() const
    {
        return (m_colorMode != PaintBrushColorMode::Greyscale) && (m_brushMode != PaintBrushMode::Smooth);
    }

    bool GlobalPaintBrushSettings::GetIntensityVisibility() const
    {
        return (m_colorMode == PaintBrushColorMode::Greyscale) && (m_brushMode != PaintBrushMode::Smooth);
    }

    void GlobalPaintBrushSettings::SetBrushMode(PaintBrushMode brushMode)
    {
        m_brushMode = brushMode;

        OnBrushModeChanged();
        OnSettingsChanged();
    }

    void GlobalPaintBrushSettings::SetColorMode(PaintBrushColorMode colorMode)
    {
        m_colorMode = colorMode;

        OnColorModeChanged();
        OnSettingsChanged();
    }

} // namespace AzToolsFramework
