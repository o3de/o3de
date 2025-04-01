/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <EditorComponents/EditorDiffuseGlobalIlluminationComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorDiffuseGlobalIlluminationComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDiffuseGlobalIlluminationComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDiffuseGlobalIlluminationComponent>(
                        "Diffuse Global Illumination", "Diffuse Global Illumination configuration")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Graphics/Lighting")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://")
                        ;

                    editContext->Class<DiffuseGlobalIlluminationComponentController>(
                        "ToneMapperComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DiffuseGlobalIlluminationComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<DiffuseGlobalIlluminationComponentConfig>("DiffuseGlobalIlluminationComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->DataElement(Edit::UIHandlers::ComboBox, &DiffuseGlobalIlluminationComponentConfig::m_qualityLevel, "Quality Level", "Quality Level")
                            ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->EnumAttribute(DiffuseGlobalIlluminationQualityLevel::Low, "Low")
                            ->EnumAttribute(DiffuseGlobalIlluminationQualityLevel::Medium, "Medium")
                            ->EnumAttribute(DiffuseGlobalIlluminationQualityLevel::High, "High")
                    ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorDiffuseGlobalIlluminationComponentTypeId", BehaviorConstant(Uuid(EditorDiffuseGlobalIlluminationComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorDiffuseGlobalIlluminationComponent::EditorDiffuseGlobalIlluminationComponent(const DiffuseGlobalIlluminationComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorDiffuseGlobalIlluminationComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
