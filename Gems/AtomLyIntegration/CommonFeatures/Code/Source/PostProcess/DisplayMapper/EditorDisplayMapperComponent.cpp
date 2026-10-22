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
#include <AzFramework/Translation/TranslationDef.h>

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
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Display Mapper"), QT_TRANSLATE_NOOP("AtomLyIntegration", "The display mapper applying on the look modification process."))
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO][ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO][ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level"), AZ_CRC_CE("Game") }))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/display-mapper/") // [GFX TODO][ATOM-2672][PostFX] need to create page for PostProcessing.
                        ;

                    editContext->Class<DisplayMapperComponentController>(
                        "ToneMapperComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DisplayMapperComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<AcesParameterOverrides>(
                        "AcesParameterOverrides", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        // m_overrideDefaults
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &AcesParameterOverrides::m_overrideDefaults, QT_TRANSLATE_NOOP("AtomLyIntegration", "Override Defaults"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "When enabled allows parameter overrides for ACES configuration"))
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_alterSurround
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &AcesParameterOverrides::m_alterSurround, QT_TRANSLATE_NOOP("AtomLyIntegration", "Alter Surround"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Apply gamma adjustment to compensate for dim surround"))
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_applyDesaturation
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &AcesParameterOverrides::m_applyDesaturation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Alter Desaturation"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Apply desaturation to compensate for luminance difference"))
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_applyCATD60toD65
                        ->DataElement(
                            AZ::Edit::UIHandlers::CheckBox, &AcesParameterOverrides::m_applyCATD60toD65, QT_TRANSLATE_NOOP("AtomLyIntegration", "Alter CAT D60 to D65"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Apply Color appearance transform (CAT) from ACES white point to assumed observer adapted white point"))
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_cinemaLimitsBlack
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AcesParameterOverrides::m_cinemaLimitsBlack,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Cinema Limit (black)"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Reference black luminance value"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.02f)
                        ->Attribute(AZ::Edit::Attributes::Max, &AcesParameterOverrides::m_cinemaLimitsWhite)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.005f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_cinemaLimitsWhite
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AcesParameterOverrides::m_cinemaLimitsWhite,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Cinema Limit (white)"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Reference white luminance value"))
                        ->Attribute(AZ::Edit::Attributes::Min, &AcesParameterOverrides::m_cinemaLimitsBlack)
                        ->Attribute(AZ::Edit::Attributes::Max, 4000.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.005f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_minPoint
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AcesParameterOverrides::m_minPoint, QT_TRANSLATE_NOOP("AtomLyIntegration", "Min Point (luminance)"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Linear extension below this"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.002f)
                        ->Attribute(AZ::Edit::Attributes::Max, &AcesParameterOverrides::m_midPoint)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)

                        // m_midPoint
                        ->DataElement(Edit::UIHandlers::Slider, &AcesParameterOverrides::m_midPoint,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Mid Point (luminance)"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Middle gray"))
                        ->Attribute(AZ::Edit::Attributes::Min, &AcesParameterOverrides::m_minPoint)
                        ->Attribute(AZ::Edit::Attributes::Max, &AcesParameterOverrides::m_maxPoint)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)

                        // m_maxPoint
                        ->DataElement(
                            Edit::UIHandlers::Slider, &AcesParameterOverrides::m_maxPoint, QT_TRANSLATE_NOOP("AtomLyIntegration", "Max Point (luminance)"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Linear extension above this"))
                        ->Attribute(AZ::Edit::Attributes::Min, &AcesParameterOverrides::m_midPoint)
                        ->Attribute(AZ::Edit::Attributes::Max, 4000.f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)

                        // m_surroundGamma
                        ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &AcesParameterOverrides::m_surroundGamma, QT_TRANSLATE_NOOP("AtomLyIntegration", "Surround Gamma"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Gamma adjustment to be applied to compensate for the condition of the viewing environment"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.6f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.2f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.005f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // m_gamma
                        ->DataElement(
                            AZ::Edit::UIHandlers::Slider, &AcesParameterOverrides::m_gamma, QT_TRANSLATE_NOOP("AtomLyIntegration", "Gamma"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Optional gamma value that is applied as basic gamma curve OETF"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.2f)
                        ->Attribute(AZ::Edit::Attributes::Max, 4.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.005f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // Load preset group
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Load Preset"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(
                            Edit::UIHandlers::ComboBox, &AcesParameterOverrides::m_preset, QT_TRANSLATE_NOOP("AtomLyIntegration", "Preset Selection"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Allows specifying default preset for different ODT modes"))
                        ->EnumAttribute(OutputDeviceTransformType::OutputDeviceTransformType_48Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "48 Nits"))
                        ->EnumAttribute(OutputDeviceTransformType::OutputDeviceTransformType_1000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "1000 Nits"))
                        ->EnumAttribute(OutputDeviceTransformType::OutputDeviceTransformType_2000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "2000 Nits"))
                        ->EnumAttribute(OutputDeviceTransformType::OutputDeviceTransformType_4000Nits, QT_TRANSLATE_NOOP("AtomLyIntegration", "4000 Nits"))
                        ->UIElement(AZ::Edit::UIHandlers::Button, QT_TRANSLATE_NOOP("AtomLyIntegration", "Load"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Load default preset"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AcesParameterOverrides::LoadPreset)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, QT_TRANSLATE_NOOP("AtomLyIntegration", "Load"))
                        ;

                    editContext->Class<DisplayMapperComponentConfig>("ToneMapperComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DisplayMapperComponentConfig::m_displayMapperOperation, QT_TRANSLATE_NOOP("AtomLyIntegration", "Type"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Display Mapper Type."))
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<DisplayMapperOperationType>())
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DisplayMapperComponentConfig::m_ldrColorGradingLutEnabled,
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable LDR color grading LUT"),
                            QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable LDR color grading LUT."))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DisplayMapperComponentConfig::m_ldrColorGradingLut, QT_TRANSLATE_NOOP("AtomLyIntegration", "LDR color Grading LUT"), QT_TRANSLATE_NOOP("AtomLyIntegration", "LDR color grading LUT"))
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DisplayMapperComponentConfig::m_acesParameterOverrides, QT_TRANSLATE_NOOP("AtomLyIntegration", "ACES Parameters"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Parameter overrides for ACES."))
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
