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

#include <Grid/EditorGridComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorGridComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorGridComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorGridComponent>(
                        "Grid", "Adds grid to the scene")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-grid.html")
                        ;

                    editContext->Class<GridComponentController>(
                        "GridComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GridComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<GridComponentConfig>(
                        "GridComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GridComponentConfig::m_gridSize, "Grid Size", "Grid width and depth")
                            ->Attribute(AZ::Edit::Attributes::Min, GridComponentController::MinGridSize)
                            ->Attribute(AZ::Edit::Attributes::Max, GridComponentController::MaxGridSize)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GridComponentConfig::m_primarySpacing, "Primary Grid Spacing", "Amount of space between grid lines")
                            ->Attribute(AZ::Edit::Attributes::Min, GridComponentController::MinSpacing)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &GridComponentConfig::m_secondarySpacing, "Secondary Grid Spacing", "Amount of space between sub-grid lines")
                            ->Attribute(AZ::Edit::Attributes::Min, GridComponentController::MinSpacing)
                            ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &GridComponentConfig::m_axisColor, "Axis Color", "Color of the grid axis")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &GridComponentConfig::m_primaryColor, "Primary Color", "Color of the primary grid lines")
                        ->DataElement(AZ::Edit::UIHandlers::Color, &GridComponentConfig::m_secondaryColor, "Secondary Color", "Color of the secondary grid lines")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorGridComponent>()->RequestBus("GridComponentRequestBus");

                behaviorContext->ConstantProperty("EditorGridComponentTypeId", BehaviorConstant(Uuid(EditorGridComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorGridComponent::EditorGridComponent(const GridComponentConfig& config)
            : BaseClass(config)
        {
        }

    } // namespace Render
} // namespace AZ
