/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <Debug/RenderDebugEditorComponent.h>

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
                    "Debug Rendering", "Controls for debugging rendering.")
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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RenderDebugComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<RenderDebugComponentConfig>("RenderDebugComponentConfig", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_enabled,
                        "Enable Render Debugging", "Enable Render Debugging.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)

                    // Render Debug View Mode
                    ->DataElement(Edit::UIHandlers::ComboBox, &RenderDebugComponentConfig::m_renderDebugViewMode,
                        "Debug View Mode", "What debug info to output to the view.")
                        ->EnumAttribute(RenderDebugViewMode::None, "None")
                        ->EnumAttribute(RenderDebugViewMode::BaseColor, "Base Color")
                        ->EnumAttribute(RenderDebugViewMode::Albedo, "Albedo")
                        ->EnumAttribute(RenderDebugViewMode::Roughness, "Roughness")
                        ->EnumAttribute(RenderDebugViewMode::Metallic, "Metallic")
                        ->EnumAttribute(RenderDebugViewMode::Normal, "Normal")
                        ->EnumAttribute(RenderDebugViewMode::Tangent, "Tangent")
                        ->EnumAttribute(RenderDebugViewMode::Bitangent, "Bitangent")
                        ->EnumAttribute(RenderDebugViewMode::CascadeShadows, "CascadeShadows")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Lighting
                    ->ClassElement(Edit::ClassElements::Group, "Lighting")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::ComboBox, &RenderDebugComponentConfig::m_renderDebugLightingType,
                        "Lighting Type", "Controls whether diffuse or specular lighting is displayed.")
                        ->EnumAttribute(RenderDebugLightingType::DiffuseAndSpecular, "Diffuse + Specular")
                        ->EnumAttribute(RenderDebugLightingType::Diffuse, "Diffuse")
                        ->EnumAttribute(RenderDebugLightingType::Specular, "Specular")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::ComboBox, &RenderDebugComponentConfig::m_renderDebugLightingSource,
                        "Lighting Source", "Controls whether direct or indirect lighting is displayed.")
                        ->EnumAttribute(RenderDebugLightingSource::DirectAndIndirect, "Direct + Indirect")
                        ->EnumAttribute(RenderDebugLightingSource::Direct, "Direct")
                        ->EnumAttribute(RenderDebugLightingSource::Indirect, "Indirect")
                        ->EnumAttribute(RenderDebugLightingSource::DebugLight, "Debug Light")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &RenderDebugComponentConfig::m_debugLightingColor,
                        "Debug Light Color", "RGB value of the debug light if used.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsDebugLightReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_debugLightingIntensity,
                        "Debug Light Intensity", "Intensity of the debug light")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 25.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsDebugLightReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_debugLightingAzimuth,
                        "Debug Light Azimuth", "Azimuth controlling the direction of the debug light")
                        // A range of [0, 360] creates a hard edge that the user can't keep rotating along, forcing them to push the slider to the opposite side
                        // This isn't user friendly if the user wants to test lighting angles around the 0 degree mark, therefore we set the range to [-360, 360]
                        // This provides the user with two full rotations and lets them gradually test around any angle without hitting the wall mentioned above
                        ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsDebugLightReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_debugLightingElevation,
                        "Debug Light Elevation", "Elevation controlling the direction of the debug light")
                        ->Attribute(AZ::Edit::Attributes::Min, -90.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsDebugLightReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Base Color Override
                    ->ClassElement(Edit::ClassElements::Group, "Base Color")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_overrideBaseColor,
                        "Override Base Color", "Whether to override base color values on materials in the scene.")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Color, &RenderDebugComponentConfig::m_materialBaseColorOverride,
                        "Base Color Value", "RGB value used to override base color on materials in the scene.")
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsBaseColorReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    //  Roughness Override
                    ->ClassElement(Edit::ClassElements::Group, "Roughness")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_overrideRoughness,
                        "Override Roughness", "Whether to override roughness values on materials in the scene.")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_materialRoughnessOverride,
                        "Roughness Value", "Roughness value used to override materials in the scene")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsRoughnessReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Metallic Override
                    ->ClassElement(Edit::ClassElements::Group, "Metallic")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_overrideMetallic,
                        "Override Metallic", "Whether to override roughness values on materials in the scene.")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_materialMetallicOverride,
                        "Metallic Value", "Metallic value used to override materials in the scene")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &RenderDebugComponentConfig::IsMetallicReadOnly)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Normal Maps
                    ->ClassElement(Edit::ClassElements::Group, "Normals")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_enableNormalMaps,
                        "Enable Normal Maps", "Whether to use normal maps in rendering.")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_enableDetailNormalMaps,
                        "Enable Detail Normal Maps", "Whether to use detail normal maps in rendering.")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    // Custom Debug Variables
                    // Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code
                    // Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code
                    ->ClassElement(Edit::ClassElements::Group, "Custom Debug Variables")
                        ->Attribute(Edit::Attributes::AutoExpand, false)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_customDebugOption01,
                        "Custom Option 01", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_customDebugOption02,
                        "Custom Option 02", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_customDebugOption03,
                        "Custom Option 03", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(Edit::UIHandlers::CheckBox, &RenderDebugComponentConfig::m_customDebugOption04,
                        "Custom Option 04", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat01,
                        "Custom Float 01", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat02,
                        "Custom Float 02", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat03,
                        "Custom Float 03", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat04,
                        "Custom Float 04", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, -1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat05,
                        "Custom Float 05", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, -1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat06,
                        "Custom Float 06", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, -1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat07,
                        "Custom Float 07", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat08,
                        "Custom Float 08", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(Edit::Attributes::Visibility, &RenderDebugComponentConfig::GetEnabled)

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RenderDebugComponentConfig::m_customDebugFloat09,
                        "Custom Float 09", "Custom variables are accessible from the Scene SRG for shader authors to use directly in their azsl code"
                        "Please use these only for local debugging purposes and DO NOT leave their usage in when submitting code")
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
