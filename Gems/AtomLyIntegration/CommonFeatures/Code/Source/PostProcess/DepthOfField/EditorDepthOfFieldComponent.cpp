/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/DepthOfField/EditorDepthOfFieldComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorDepthOfFieldComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDepthOfFieldComponent, BaseClass>()
                    ->Version(2);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDepthOfFieldComponent>(
                        "Depth Of Field", "Controls the Depth of Field.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.y 
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/depth-of-field/") // [GFX TODO][ATOM-2672][PostFX] need create page for PostProcessing.
                        ;

                    editContext->Class<DepthOfFieldComponentController>(
                        "DepthOfFieldComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DepthOfFieldComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<DepthOfFieldComponentConfig>("DepthOfFieldComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")

                        ->DataElement(Edit::UIHandlers::EntityId,
                            &DepthOfFieldComponentConfig::m_cameraEntityId,
                            "Camera Entity",
                            "Camera entity. Required by Depth of Field.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DepthOfFieldComponentConfig::m_enabled,
                            "Enable Depth of Field",
                            "Enable Depth of Field.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::IsCameraEntityInvalid)

                        ->DataElement(Edit::UIHandlers::Slider, &DepthOfFieldComponentConfig::m_qualityLevel,
                            "Quality Level",
                            "0 : Standard Bokeh blur.\n"
                            "1 : High quality Bokeh blur (large number of sample)")
                        ->Attribute(Edit::Attributes::Min, 0)
                        ->Attribute(Edit::Attributes::Max, DepthOfField::QualityLevelMax - 1)
                        ->Attribute(Edit::Attributes::Step, 1)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &DepthOfFieldComponentConfig::m_apertureF,
                            "Aperture F",
                            "The higher the value, the larger the opening.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Default, &DepthOfFieldComponentConfig::m_fNumber,
                            "F Number",
                            "")
                        ->Attribute(Edit::Attributes::ReadOnly, true)


                        ->DataElement(Edit::UIHandlers::Default, &DepthOfFieldComponentConfig::m_focusDistance,
                            "Focus Distance",
                            "The distance from the camera to the focused subject.")
                        ->Attribute(Edit::Attributes::Suffix, " m")
                        ->Attribute(Edit::Attributes::Step, 1.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::IsFocusDistanceReadOnly)

                        // Auto Focus
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Auto Focus")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DepthOfFieldComponentConfig::m_enableAutoFocus,
                            "Enable Auto Focus",
                            "Enables auto focus.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(Edit::UIHandlers::Default,
                            &DepthOfFieldComponentConfig::m_focusedEntityId,
                            "Focused Entity",
                            "Entity to focus on.\n"
                            "If unset, the screen position is used.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::AttributesAndValues)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::IsFocusedEntityReadonly)

                        ->DataElement(Edit::UIHandlers::Default, &DepthOfFieldComponentConfig::m_autoFocusScreenPosition,
                            "Focus Screen Position",
                            "Values of the focus position on screen.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::IsAutoFocusReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &DepthOfFieldComponentConfig::m_autoFocusSensitivity,
                            "Auto Focus Sensitivity",
                            "Higher value is more responsive.\n"
                            "Lower value require a greater difference in depth before refocusing.\n"
                            "Always responds when the value is 1.0.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::IsAutoFocusReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &DepthOfFieldComponentConfig::m_autoFocusSpeed,
                            "Auto Focus Speed",
                            "Specify the distance that focus moves per second,\n"
                            "normalizing the distance from view near to view far as 1.0.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, DepthOfField::AutoFocusSpeedMax)
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::IsAutoFocusReadOnly)

                        ->DataElement(Edit::UIHandlers::Slider, &DepthOfFieldComponentConfig::m_autoFocusDelay,
                            "Auto Focus Delay",
                            "Specifies a delay time for focus to shift from one to another target.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, DepthOfField::AutoFocusDelayMax)
                        ->Attribute(AZ::Edit::Attributes::Suffix, "[s]")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &DepthOfFieldComponentConfig::IsAutoFocusReadOnly)

                        // Debugging
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Debugging")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DepthOfFieldComponentConfig::m_enableDebugColoring,
                            "Enable Debug Color",
                            "Enable coloring to see Depth of Field level\n"
                            "Red - Back large blur\n"
                            "Orange - Back middle blur\n"
                            "Yellow - Back small blur\n"
                            "Green - Focus area\n"
                            "Blue green - Front small blur\n"
                            "Blue - Front middle blur\n"
                            "Purple - Front large blur")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Overrides")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        // Auto-gen editor context settings for overrides
#define EDITOR_CLASS DepthOfFieldComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorDepthOfFieldComponent>()->RequestBus("DepthOfFieldRequestBus");

                behaviorContext->ConstantProperty("EditorDepthOfFieldComponentTypeId", BehaviorConstant(Uuid(DepthOfField::EditorDepthOfFieldComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorDepthOfFieldComponent::EditorDepthOfFieldComponent(const DepthOfFieldComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorDepthOfFieldComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
