/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/MotionBlur/EditorMotionBlurComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        using MotionBlurQualityComboBoxVec = AZStd::vector<AZ::Edit::EnumConstant<MotionBlur::SampleQuality>>;
        static MotionBlurQualityComboBoxVec PopulateMotionBlurQualityList()
        {
            return MotionBlurQualityComboBoxVec
            {
                { MotionBlur::SampleQuality::Low, "Low" },
                { MotionBlur::SampleQuality::Medium, "Medium" },
                { MotionBlur::SampleQuality::High, "High" },
                { MotionBlur::SampleQuality::Ultra, "Ultra" },
            };
        }

        void EditorMotionBlurComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorMotionBlurComponent, BaseClass>()->Version(0);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMotionBlurComponent>("Motion Blur", "Controls the Motion Blur")
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
                            "https://o3de.org/docs/user-guide/components/reference/atom/MotionBlur/") // Create documentation page for this effect
                        ;

                    editContext->Class<MotionBlurComponentController>("MotionBlurComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionBlurComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                    editContext->Class<MotionBlurComponentConfig>("MotionBlurComponentConfig", "")
                        ->DataElement(
                            Edit::UIHandlers::CheckBox, &MotionBlurComponentConfig::m_enabled, "Enable Motion Blur", "Enable Motion Blur.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(AZ::Edit::UIHandlers::Slider, &MotionBlurComponentConfig::m_strength, "Strength", "Strength of the Effect")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &MotionBlurComponentConfig::ArePropertiesReadOnly)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MotionBlurComponentConfig::m_sampleQuality, "Sample Quality", "Quality of the effect")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &PopulateMotionBlurQualityList)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(Edit::Attributes::ReadOnly, &MotionBlurComponentConfig::ArePropertiesReadOnly)

                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Overrides")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                    // Auto-gen editor context settings for overrides
#define EDITOR_CLASS MotionBlurComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorMotionBlurComponent>()->RequestBus("MotionBlurRequestBus");

                behaviorContext
                    ->ConstantProperty("EditorMotionBlurComponentTypeId", BehaviorConstant(Uuid(MotionBlur::EditorMotionBlurComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorMotionBlurComponent::EditorMotionBlurComponent(const MotionBlurComponentConfig& config)
            : BaseClass(config)
        {
        }

        AZ::u32 EditorMotionBlurComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
