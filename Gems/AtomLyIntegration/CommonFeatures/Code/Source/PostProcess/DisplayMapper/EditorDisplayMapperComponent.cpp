/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/DisplayMapper/EditorDisplayMapperComponent.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorDisplayMapperComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDisplayMapperComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDisplayMapperComponent>(
                        "Display Mapper", "The display mapper applying on the look modification process.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO][ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO][ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level", 0x9aeacc13), AZ_CRC("Game", 0x232b318c) }))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/display-mapper/") // [GFX TODO][ATOM-2672][PostFX] need to create page for PostProcessing.
                        ;

                    editContext->Class<DisplayMapperComponentController>(
                        "ToneMapperComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DisplayMapperComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<AcesParameterOverrides>(
                        "AcesParameterOverrides", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        // m_overrideDefaults
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &AcesParameterOverrides::m_overrideDefaults, "Override Defaults",
                            "When enabled allows parameter overrides for ACES configuration")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_alterSurround
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &AcesParameterOverrides::m_alterSurround, "Alter Surround",
                            "Apply gamma adjustment to compensate for dim surround")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_applyDesaturation
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &AcesParameterOverrides::m_applyDesaturation, "Alter Desaturation",
                            "Apply desaturation to compensate for luminance difference")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_applyCATD60toD65
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &AcesParameterOverrides::m_applyCATD60toD65, "Alter CAT D60 to D65",
                            "Apply Color appearance transform (CAT) from ACES white point to assumed observer adapted white point")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_cinemaLimitsBlack
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AcesParameterOverrides::m_cinemaLimitsBlack,
                            "Cinema Limit (black)",
                            "Reference black luminance value")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.02f)
                        ->Attribute(AZ::Edit::Attributes::Max, &AcesParameterOverrides::m_cinemaLimitsWhite)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.005f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_cinemaLimitsWhite
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AcesParameterOverrides::m_cinemaLimitsWhite,
                            "Cinema Limit (white)",
                            "Reference white luminance value")
                        ->Attribute(AZ::Edit::Attributes::Min, &AcesParameterOverrides::m_cinemaLimitsBlack)
                        ->Attribute(AZ::Edit::Attributes::Max, 4000.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.005f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_minPoint
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AcesParameterOverrides::m_minPoint, "Min Point (luminance)",
                            "Linear extension below this")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.002f)
                        ->Attribute(AZ::Edit::Attributes::Max, &AcesParameterOverrides::m_midPoint)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)

                        // m_midPoint
                        ->DataElement(Edit::UIHandlers::Slider, &AcesParameterOverrides::m_midPoint,
                            "Mid Point (luminance)", "Middle gray")
                        ->Attribute(AZ::Edit::Attributes::Min, &AcesParameterOverrides::m_minPoint)
                        ->Attribute(AZ::Edit::Attributes::Max, &AcesParameterOverrides::m_maxPoint)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)

                        // m_maxPoint
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AcesParameterOverrides::m_maxPoint, "Max Point (luminance)",
                            "Linear extension above this")
                        ->Attribute(AZ::Edit::Attributes::Min, &AcesParameterOverrides::m_midPoint)
                        ->Attribute(AZ::Edit::Attributes::Max, 4000.f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)

                        // m_surroundGamma
                        ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &AcesParameterOverrides::m_surroundGamma, "Surround Gamma",
                            "Gamma adjustment to be applied to compensate for the condition of the viewing environment")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.6f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.2f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.005f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_gamma
                        ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &AcesParameterOverrides::m_gamma, "Gamma",
                            "Optional gamma value that is applied as basic gamma curve OETF")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.2f)
                        ->Attribute(AZ::Edit::Attributes::Max, 4.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.005f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // Load preset group
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Load Preset")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(
                            Edit::UIHandlers::ComboBox, &AcesParameterOverrides::m_preset, "Preset Selection",
                            "Allows specifying default preset for different ODT modes")
                        ->EnumAttribute(OutputDeviceTransformType::OutputDeviceTransformType_48Nits, "48 Nits")
                        ->EnumAttribute(OutputDeviceTransformType::OutputDeviceTransformType_1000Nits, "1000 Nits")
                        ->EnumAttribute(OutputDeviceTransformType::OutputDeviceTransformType_2000Nits, "2000 Nits")
                        ->EnumAttribute(OutputDeviceTransformType::OutputDeviceTransformType_4000Nits, "4000 Nits")
                        ->UIElement(AZ::Edit::UIHandlers::Button, "Load", "Load default preset")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AcesParameterOverrides::LoadPreset)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Load")
                        ;

                    editContext->Class<DisplayMapperComponentConfig>("ToneMapperComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->DataElement(Edit::UIHandlers::ComboBox,
                            &DisplayMapperComponentConfig::m_displayMapperOperation,
                            "Type",
                            "Display Mapper Type.")
                        ->EnumAttribute(DisplayMapperOperationType::Aces, "Aces")
                        ->EnumAttribute(DisplayMapperOperationType::AcesLut, "AcesLut")
                        ->EnumAttribute(DisplayMapperOperationType::Passthrough, "Passthrough")
                        ->EnumAttribute(DisplayMapperOperationType::GammaSRGB, "GammaSRGB")
                        ->EnumAttribute(DisplayMapperOperationType::Reinhard, "Reinhard")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DisplayMapperComponentConfig::m_ldrColorGradingLutEnabled,
                            "Enable LDR color grading LUT",
                            "Enable LDR color grading LUT.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DisplayMapperComponentConfig::m_ldrColorGradingLut, "LDR color Grading LUT", "LDR color grading LUT")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DisplayMapperComponentConfig::m_acesParameterOverrides, "ACES Parameters", "Parameter overrides for ACES.")
                    ;

                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorDisplayMapperComponent>()->RequestBus("DisplayMapperComponentRequestBus");

                behaviorContext->ConstantProperty("EditorDisplayMapperComponentTypeId", BehaviorConstant(Uuid(EditorDisplayMapperComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorDisplayMapperComponent::EditorDisplayMapperComponent(const DisplayMapperComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorDisplayMapperComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
