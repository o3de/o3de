/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <PostProcess/GradientWeightModifier/EditorGradientWeightModifierComponent.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/GradientWeightModifier/GradientWeightModifierComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        void EditorGradientWeightModifierComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorGradientWeightModifierComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorGradientWeightModifierComponent>(
                        "PostFX Gradient Weight Modifier", "Modifies PostFX override factor based on a gradient signal sampled from an entity")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "")
                        ;

                    editContext->Class<GradientWeightModifierComponentController>("GradientWeightModifierComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GradientWeightModifierComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<GradientWeightModifierComponentConfig>("GradientWeightModifierComponentConfig", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GradientWeightModifierComponentConfig::m_gradientSampler, "Gradient Sampler", "Gradient sampler configuration")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorGradientWeightModifierComponent>()->RequestBus("PostFxWeightRequestBus");

                behaviorContext->ConstantProperty("EditorGradientWeightModifierComponentTypeId", BehaviorConstant(Uuid(EditorGradientWeightModifierComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorGradientWeightModifierComponent::EditorGradientWeightModifierComponent(const GradientWeightModifierComponentConfig& config)
            : BaseClass(config)
        {

        }

        AZ::u32 EditorGradientWeightModifierComponent::OnConfigurationChanged()
        {
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}

