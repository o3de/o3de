/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/Ssao/EditorSsaoComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorSsaoComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorSsaoComponent, BaseClass>()
                    ->Version(2);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorSsaoComponent>(
                        "SSAO", "Controls SSAO.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                            ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/ssao/") // [GFX TODO][ATOM-2672][PostFX] need create page for PostProcessing.
                        ;

                    editContext->Class<SsaoComponentController>(
                        "SsaoComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &SsaoComponentController::m_configuration, "Configuration", "")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<SsaoComponentConfig>("SsaoComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &SsaoComponentConfig::m_enabled,
                            "Enable SSAO",
                            "Enable SSAO.")

                        ->DataElement(Edit::UIHandlers::Slider, &SsaoComponentConfig::m_strength,
                            "SSAO Strength",
                            "Multiplier for how much strong SSAO appears.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 2.0f)

                        ->DataElement(Edit::UIHandlers::Slider, &SsaoComponentConfig::m_samplingRadius,
                            "Sampling Radius",
                            "The sampling radius of the SSAO effect in screen UV space")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 0.25f)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &SsaoComponentConfig::m_enableBlur,
                            "Enable Blur",
                            "Enables SSAO Blur")

                        ->DataElement(Edit::UIHandlers::Slider, &SsaoComponentConfig::m_blurConstFalloff,
                            "Blur Strength",
                            "Affects how strong the blur is. Recommended value is 0.67")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 0.95f)

                        ->DataElement(Edit::UIHandlers::Slider, &SsaoComponentConfig::m_blurDepthFalloffStrength,
                            "Blur Sharpness",
                            "Affects how sharp the SSAO blur appears around edges. Recommended value is 50")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 400.0f)

                        ->DataElement(Edit::UIHandlers::Slider, &SsaoComponentConfig::m_blurDepthFalloffThreshold,
                            "Blur Edge Threshold",
                            "Affects the threshold needed for the blur algorithm to detect an edge. Recommended to be left at 0.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &SsaoComponentConfig::m_enableDownsample,
                            "Enable Downsample",
                            "Enables depth downsampling before SSAO. Slightly lower quality but 2x as fast as regular SSAO.")


                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Overrides")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        // Auto-gen editor context settings for overrides
#define EDITOR_CLASS SsaoComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/Ssao/SsaoParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorSsaoComponent>()->RequestBus("SsaoRequestBus");

                behaviorContext->ConstantProperty("EditorSsaoComponentTypeId", BehaviorConstant(Uuid(Ssao::EditorSsaoComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorSsaoComponent::EditorSsaoComponent(const SsaoComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorSsaoComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
