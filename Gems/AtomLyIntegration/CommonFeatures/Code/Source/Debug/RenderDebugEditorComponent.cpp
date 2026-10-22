/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <Debug/RenderDebugEditorComponent.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace AZ::Render
{
    void RenderDebugEditorComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RenderDebugEditorComponent, BaseClass>()
                ->Version(0);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<RenderDebugEditorComponent>(
                    QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug Rendering"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls for debugging rendering."))
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/Debugging")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;

                editContext->Class<RenderDebugComponentController>(
                    "RenderDebugComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RenderDebugComponentController::m_configuration, QT_TRANSLATE_NOOP("AtomLyIntegration", "Configuration"), "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<RenderDebugComponentConfig>("RenderDebugComponentConfig", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_enabled,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Render Debugging"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Render Debugging."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)

                    // Render Debug View Mode
                    ->DataElement(Edit::UIHandlers::ComboBox, &RenderDebugComponentConfig::m_renderDebugViewMode,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug View Mode"), QT_TRANSLATE_NOOP("AtomLyIntegration", "What debug info to output to the view."))
                        ->EnumAttribute(RenderDebugViewMode::None, QT_TRANSLATE_NOOP("AtomLyIntegration", "None"))
                        ->EnumAttribute(RenderDebugViewMode::BaseColor, QT_TRANSLATE_NOOP("AtomLyIntegration", "Base Color"))
                        ->EnumAttribute(RenderDebugViewMode::Albedo, QT_TRANSLATE_NOOP("AtomLyIntegration", "Albedo"))
                        ->EnumAttribute(RenderDebugViewMode::Roughness, QT_TRANSLATE_NOOP("AtomLyIntegration", "Roughness"))
                        ->EnumAttribute(RenderDebugViewMode::Metallic, QT_TRANSLATE_NOOP("AtomLyIntegration", "Metallic"))
                        ->EnumAttribute(RenderDebugViewMode::Normal, QT_TRANSLATE_NOOP("AtomLyIntegration", "Normal"))
                        ->EnumAttribute(RenderDebugViewMode::Tangent, QT_TRANSLATE_NOOP("AtomLyIntegration", "Tangent"))
                        ->EnumAttribute(RenderDebugViewMode::Bitangent, QT_TRANSLATE_NOOP("AtomLyIntegration", "Bitangent"))
                        ->EnumAttribute(RenderDebugViewMode::CascadeShadows, QT_TRANSLATE_NOOP("AtomLyIntegration", "CascadeShadows"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Lighting
                    ->ClassElement(Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Lighting"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::ComboBox, &RenderDebugComponentConfig::m_renderDebugLightingType,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Lighting Type"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls whether diffuse or specular lighting is displayed."))
                        ->EnumAttribute(RenderDebugLightingType::DiffuseAndSpecular, QT_TRANSLATE_NOOP("AtomLyIntegration", "Diffuse + Specular"))
                        ->EnumAttribute(RenderDebugLightingType::Diffuse, QT_TRANSLATE_NOOP("AtomLyIntegration", "Diffuse"))
                        ->EnumAttribute(RenderDebugLightingType::Specular, QT_TRANSLATE_NOOP("AtomLyIntegration", "Specular"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::ComboBox, &RenderDebugComponentConfig::m_renderDebugLightingSource,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Lighting Source"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Controls whether direct or indirect lighting is displayed."))
                        ->EnumAttribute(RenderDebugLightingSource::DirectAndIndirect, QT_TRANSLATE_NOOP("AtomLyIntegration", "Direct + Indirect"))
                        ->EnumAttribute(RenderDebugLightingSource::Direct, QT_TRANSLATE_NOOP("AtomLyIntegration", "Direct"))
                        ->EnumAttribute(RenderDebugLightingSource::Indirect, QT_TRANSLATE_NOOP("AtomLyIntegration", "Indirect"))
                        ->EnumAttribute(RenderDebugLightingSource::DebugLight, QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug Light"))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &RenderDebugComponentConfig::m_debugLightingColor,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug Light Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "RGB value of the debug light if used."))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsDebugLightReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_debugLightingIntensity,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug Light Intensity"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Intensity of the debug light"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 25.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsDebugLightReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_debugLightingAzimuth,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug Light Azimuth"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Azimuth controlling the direction of the debug light"))
                        // A range of [0, 360] creates a hard edge that the user can't keep rotating along, forcing them to push the slider to the opposite side
                        // This isn't user friendly if the user wants to test lighting angles around the 0 degree mark, therefore we set the range to [-360, 360]
                        // This provides the user with two full rotations and lets them gradually test around any angle without hitting the wall mentioned above
                        ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsDebugLightReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_debugLightingElevation,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Debug Light Elevation"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Elevation controlling the direction of the debug light"))
                        ->Attribute(AZ::Edit::Attributes::Min, -90.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsDebugLightReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Base Color Override
                    ->ClassElement(Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Base Color"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_overrideBaseColor,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Override Base Color"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Whether to override base color values on materials in the scene."))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &RenderDebugComponentConfig::m_materialBaseColorOverride,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Base Color Value"), QT_TRANSLATE_NOOP("AtomLyIntegration", "RGB value used to override base color on materials in the scene."))
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsBaseColorReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    //  Roughness Override
                    ->ClassElement(Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Roughness"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_overrideRoughness,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Override Roughness"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Whether to override roughness values on materials in the scene."))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_materialRoughnessOverride,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Roughness Value"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Roughness value used to override materials in the scene"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsRoughnessReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Metallic Override
                    ->ClassElement(Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Metallic"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_overrideMetallic,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Override Metallic"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Whether to override roughness values on materials in the scene."))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_materialMetallicOverride,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Metallic Value"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Metallic value used to override materials in the scene"))
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsMetallicReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Normal Maps
                    ->ClassElement(Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Normals"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_enableNormalMaps,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Normal Maps"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Whether to use normal maps in rendering."))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_enableDetailNormalMaps,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Enable Detail Normal Maps"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Whether to use detail normal maps in rendering."))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Custom Debug Variables
                    // Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code
                    // Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code
                    ->ClassElement(Edit::ClassElements::Group, QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Debug Variables"))
                        ->Attribute(Edit::Attributes::AutoExpand, false)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_customDebugOption01,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Option 01"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_customDebugOption02,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Option 02"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_customDebugOption03,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Option 03"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_customDebugOption04,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Option 04"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat01,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 01"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat02,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 02"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat03,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 03"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat04,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 04"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, -1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat05,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 05"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, -1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat06,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 06"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, -1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat07,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 07"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat08,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 08"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat09,
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom Float 09"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code"))
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<RenderDebugEditorComponent>()->RequestBus("RenderDebugRequestBus");

            behaviorContext->ConstantProperty("RenderDebugEditorComponentTypeId", BehaviorConstant(Uuid(RenderDebug::RenderDebugEditorComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }

    RenderDebugEditorComponent::RenderDebugEditorComponent(const RenderDebugComponentConfig& config)
        : BaseClass(config)
    {
    }

    u32 RenderDebugEditorComponent::OnConfigurationChanged()
    {
        m_controller.OnConfigChanged();
        return Edit::PropertyRefreshLevels::AttributesAndValues;
    }

}
