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

#include <AzCore/RTTI/BehaviorContext.h>
#include <PostProcess/DisplayMapper/EditorDisplayMapperComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorDisplayMapperComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDisplayMapperComponent, BaseClass>()
                    ->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDisplayMapperComponent>(
                        "Display Mapper", "The display mapper applying on the look modification process.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO][ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.png") // [GFX TODO][ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level", 0x9aeacc13), AZ_CRC("Game", 0x232b318c) }))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://") // [GFX TODO][ATOM-2672][PostFX] need to create page for PostProcessing.
                        ;

                    editContext->Class<DisplayMapperComponentController>(
                        "ToneMapperComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DisplayMapperComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<DisplayMapperComponentConfig>("ToneMapperComponentConfig", "")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->DataElement(Edit::UIHandlers::ComboBox,
                            &DisplayMapperComponentConfig::m_displayMapperOperation,
                            "Type",
                            "Display Mapper Type.")
                        ->EnumAttribute(DisplayMapperOperationType::Aces, "Aces")
                        ->EnumAttribute(DisplayMapperOperationType::AcesLut, "AcesLut")
                        ->EnumAttribute(DisplayMapperOperationType::Passthrough, "Passthrough")
                        ->EnumAttribute(DisplayMapperOperationType::GammaSRGB, "GammaSRGB")
                        ->EnumAttribute(DisplayMapperOperationType::Reinhard, "Reinhard")
                        ->Attribute(Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)

                        ->DataElement(Edit::UIHandlers::CheckBox,
                            &DisplayMapperComponentConfig::m_ldrColorGradingLutEnabled,
                            "Enable LDR color grading LUT",
                            "Enable LDR color grading LUT.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DisplayMapperComponentConfig::m_ldrColorGradingLut, "LDR color Grading LUT", "LDR color grading LUT");
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorDisplayMapperComponentTypeId", BehaviorConstant(Uuid(EditorDisplayMapperComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorDisplayMapperComponent::EditorDisplayMapperComponent(const DisplayMapperComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorDisplayMapperComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
