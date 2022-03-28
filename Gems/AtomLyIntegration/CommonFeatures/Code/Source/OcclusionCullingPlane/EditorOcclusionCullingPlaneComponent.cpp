/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OcclusionCullingPlane/EditorOcclusionCullingPlaneComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Component/Entity.h>

namespace AZ
{
    namespace Render
    {
        void EditorOcclusionCullingPlaneComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorOcclusionCullingPlaneComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>)
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorOcclusionCullingPlaneComponent>(
                        "Occlusion Culling Plane", "The OcclusionCullingPlane component is used to cull meshes that are inside the view frustum and behind the occlusion plane")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/occlusion-culling-plane/")
                        ;

                    editContext->Class<OcclusionCullingPlaneComponentController>(
                        "OcclusionCullingPlaneComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &OcclusionCullingPlaneComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<OcclusionCullingPlaneComponentConfig>(
                        "OcclusionCullingPlaneComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Settings")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &OcclusionCullingPlaneComponentConfig::m_showVisualization, "Show Visualization", "Show the occlusion culling plane visualization")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &OcclusionCullingPlaneComponentConfig::m_transparentVisualization, "Transparent Visualization", "Sets the occlusion culling plane visualization as transparent")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorOcclusionCullingPlaneComponentTypeId", BehaviorConstant(Uuid(EditorOcclusionCullingPlaneComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorOcclusionCullingPlaneComponent::EditorOcclusionCullingPlaneComponent()
        {
        }

        EditorOcclusionCullingPlaneComponent::EditorOcclusionCullingPlaneComponent(const OcclusionCullingPlaneComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorOcclusionCullingPlaneComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        }

        void EditorOcclusionCullingPlaneComponent::Deactivate()
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }
    } // namespace Render
} // namespace AZ
