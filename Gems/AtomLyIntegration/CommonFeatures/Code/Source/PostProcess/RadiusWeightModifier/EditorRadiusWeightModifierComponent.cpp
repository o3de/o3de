/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/RadiusWeightModifier/EditorRadiusWeightModifierComponent.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        void EditorRadiusWeightModifierComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorRadiusWeightModifierComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorRadiusWeightModifierComponent>(
                        "PostFX Radius Weight Modifier", "Modifies PostFX override factor based on proximity of an influencer against this entity's bounding sphere")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/radius-weight-modifier/")
                        ;

                    editContext->Class<RadiusWeightModifierComponentController>("RadiusWeightModifierComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RadiusWeightModifierComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<RadiusWeightModifierComponentConfig>("RadiusWeightModifierComponentConfig", "")
                        ->DataElement(AZ::Edit::UIHandlers::Slider,
                            &RadiusWeightModifierComponentConfig::m_radius,
                            "Radius",
                            "Radius of PostFx Volume.")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f);
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorRadiusWeightModifierComponent>()->RequestBus("PostFxWeightRequestBus");

                behaviorContext->ConstantProperty("EditorRadiusWeightModifierComponentTypeId", BehaviorConstant(Uuid(EditorRadiusWeightModifierComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorRadiusWeightModifierComponent::EditorRadiusWeightModifierComponent(const RadiusWeightModifierComponentConfig& config)
            : BaseClass(config)
        {

        }

        AZ::u32 EditorRadiusWeightModifierComponent::OnConfigurationChanged()
        {
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}

