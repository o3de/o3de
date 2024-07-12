/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/Vignette/EditorVignetteComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorVignetteComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorVignetteComponent, BaseClass>()->Version(0);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorVignetteComponent>("Vignette", "Controls the Vignette")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/PostFX")
                        ->Attribute(
                            AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // Create better icons for this effect
                        ->Attribute(
                            AZ::Edit::Attributes::ViewportIcon,
                            "Icons/Components/Viewport/Component_Placeholder.svg") // Create better icons for this effect
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(
                            Edit::Attributes::HelpPageURL,
                            "https://o3de.org/docs/user-guide/components/reference/atom/Vignette/") // Create documentation page for this effect
                        ;

                    editContext->Class<VignetteComponentController>("VignetteComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &VignetteComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    editContext->Class<VignetteComponentConfig>("VignetteComponentConfig", "")
                        ->DataElement(
                            Edit::UIHandlers::CheckBox, &VignetteComponentConfig::m_enabled, "Enable Vignette", "Enable Vignette.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &VignetteComponentConfig::m_intensity, "Intensity", "Intensity of effect")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &VignetteComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Overrides")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    // Auto-gen editor context settings for overrides
#define EDITOR_CLASS VignetteComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/Vignette/VignetteParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorVignetteComponent>()->RequestBus("VignetteRequestBus");

                behaviorContext
                    ->ConstantProperty("EditorVignetteComponentTypeId", BehaviorConstant(Uuid(Vignette::EditorVignetteComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorVignetteComponent::EditorVignetteComponent(const VignetteComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorVignetteComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
