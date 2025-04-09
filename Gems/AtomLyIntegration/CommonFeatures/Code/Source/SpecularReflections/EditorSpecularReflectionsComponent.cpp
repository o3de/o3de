/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <SpecularReflections/EditorSpecularReflectionsComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorSpecularReflectionsComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorSpecularReflectionsComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorSpecularReflectionsComponent>(
                        "Specular Reflections", "Specular Reflections configuration")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::HelpPageURL, "https://")
                    ;

                    editContext->Class<SpecularReflectionsComponentController>(
                        "SpecularReflectionsComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpecularReflectionsComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                    editContext->Class<SpecularReflectionsComponentSSRConfig>("Screen Space Reflections (SSR)", "Screen Space Reflections (SSR) Configuration")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpecularReflectionsComponentSSRConfig::m_options, "SSR Options", "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                    editContext->Class<SpecularReflectionsComponentConfig>("Specular Reflections Component", "Configures specular reflection options for the level")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpecularReflectionsComponentConfig::m_ssr, "SSR configuration", "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorSpecularReflectionsComponentTypeId", BehaviorConstant(Uuid(EditorSpecularReflectionsComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorSpecularReflectionsComponent::EditorSpecularReflectionsComponent(const SpecularReflectionsComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorSpecularReflectionsComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
