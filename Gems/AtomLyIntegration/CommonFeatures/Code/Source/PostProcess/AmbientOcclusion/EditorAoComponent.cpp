/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/AmbientOcclusion/EditorAoComponent.h>

namespace AZ
{
    namespace Render
    {
        using AoMethodComboBoxVec = AZStd::vector<AZ::Edit::EnumConstant<Ao::AoMethodType>>;
        static AoMethodComboBoxVec PopulateAoMethodList()
        {
            return AoMethodComboBoxVec
            {
                { Ao::AoMethodType::SSAO, "SSAO" },
                { Ao::AoMethodType::GTAO, "GTAO" },
            };
        }

        using GtaoQualityLevelComboBoxVec = AZStd::vector<AZ::Edit::EnumConstant<Ao::GtaoQualityLevel>>;
        static GtaoQualityLevelComboBoxVec PopulateGtaoQualityLevelList()
        {
            return GtaoQualityLevelComboBoxVec
            {
                { Ao::GtaoQualityLevel::SuperLow, "Super Low" },
                { Ao::GtaoQualityLevel::Low, "Low" },
                { Ao::GtaoQualityLevel::Medium, "Medium" },
                { Ao::GtaoQualityLevel::High, "High" },
                { Ao::GtaoQualityLevel::SuperHigh, "Super High" },
            };
        }

        void EditorAoComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorAoComponent, BaseClass>()
                    ->Version(2);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorAoComponent>(
                        "Ambient Occlusion", "Controls Ambient Occlusion.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::Category, "Graphics/PostFX")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                            ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/components/reference/atom/ssao/") // [GFX TODO][ATOM-2672][PostFX] need create page for PostProcessing.
                        ;

                    editContext->Class<AoComponentController>(
                        "AoComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &AoComponentController::m_configuration, "Configuration", "")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<AoComponentConfig>("AoComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &AoComponentConfig::m_enabled,
                            "Enable AO",
                            "Enable AO.")

                        ->DataElement(Edit::UIHandlers::ComboBox,
                            &AoComponentConfig::m_aoMethod,
                            "AO Method",
                            "The method used for AO calculation.")
                            ->Attribute(Edit::Attributes::EnumValues, &PopulateAoMethodList)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_ssaoStrength,
                            "SSAO Strength",
                            "Multiplier for how much strong SSAO appears.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 2.0f)
                            ->Attribute(Edit::Attributes::ReadOnly, &AoComponentConfig::IsSsaoInactive)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_ssaoSamplingRadius,
                            "SSAO Sampling Radius",
                            "The sampling radius of the SSAO effect in screen UV space")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 0.25f)
                            ->Attribute(Edit::Attributes::ReadOnly, &AoComponentConfig::IsSsaoInactive)

                        ->DataElement(Edit::UIHandlers::ComboBox,
                            &AoComponentConfig::m_gtaoQuality,
                            "GTAO Quality",
                            "The quality level for the GTAO effect.")
                            ->Attribute(Edit::Attributes::EnumValues, &PopulateGtaoQualityLevelList)
                            ->Attribute(Edit::Attributes::ReadOnly, &AoComponentConfig::IsGtaoInactive)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_gtaoStrength,
                            "GTAO Strength",
                            "Multiplier for how much strong GTAO appears.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 2.0f)
                            ->Attribute(Edit::Attributes::ReadOnly, &AoComponentConfig::IsGtaoInactive)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_gtaoPower,
                            "GTAO Power",
                            "Power factor for how much strong GTAO appears.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 5.0f)
                            ->Attribute(Edit::Attributes::ReadOnly, &AoComponentConfig::IsGtaoInactive)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_gtaoWorldRadius,
                            "GTAO World Radius",
                            "Sampling radius in world units.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 5.0f)
                            ->Attribute(Edit::Attributes::ReadOnly, &AoComponentConfig::IsGtaoInactive)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_gtaoThicknessBlend,
                            "GTAO Thickness Blend",
                            "Blend factor for thickness.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1.0f)
                            ->Attribute(Edit::Attributes::ReadOnly, &AoComponentConfig::IsGtaoInactive)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_gtaoMaxDepth,
                            "GTAO Max Depth",
                            "Max depth for GTAO effect.")
                            ->Attribute(Edit::Attributes::Min, 0.0f)
                            ->Attribute(Edit::Attributes::Max, 1000.0f)
                            ->Attribute(Edit::Attributes::ReadOnly, &AoComponentConfig::IsGtaoInactive)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &AoComponentConfig::m_enableBlur,
                            "Enable Blur",
                            "Enables AO Blur")

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_blurConstFalloff,
                            "Blur Strength",
                            "Affects how strong the blur is. Recommended value is 0.67")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 0.95f)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_blurDepthFalloffStrength,
                            "Blur Sharpness",
                            "Affects how sharp the SSAO blur appears around edges. Recommended value is 50")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 400.0f)

                        ->DataElement(Edit::UIHandlers::Slider, &AoComponentConfig::m_blurDepthFalloffThreshold,
                            "Blur Edge Threshold",
                            "Affects the threshold needed for the blur algorithm to detect an edge. Recommended to be left at 0.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &AoComponentConfig::m_enableDownsample,
                            "Enable Downsample",
                            "Enables depth downsampling before SSAO. Slightly lower quality but 2x as fast as regular SSAO.")


                        // Overrides
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Overrides")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        // Auto-gen editor context settings for overrides
#define EDITOR_CLASS AoComponentConfig
#include <Atom/Feature/ParamMacros/StartOverrideEditorContext.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef EDITOR_CLASS
                            ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorAoComponent>()->RequestBus("AoRequestBus");

                behaviorContext->ConstantProperty("EditorAoComponentTypeId", BehaviorConstant(Uuid(Ao::EditorAoComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorAoComponent::EditorAoComponent(const AoComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorAoComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
