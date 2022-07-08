/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/EditorPostFxLayerComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorPostFxLayerComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPostFxLayerComponent, BaseClass>()
                    ->Version(4);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorPostFxLayerComponent>(
                        "PostFX Layer", "This component enables the entity to specify post process settings")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/postfx-layer/")
                        ;

                    editContext->Class<PostFxLayerComponentController>(
                        "PostFxLayerComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PostFxLayerComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<PostFxLayerComponentConfig>(
                        "PostFxLayerComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->DataElement(Edit::UIHandlers::ComboBox, &PostFxLayerComponentConfig::m_layerCategoryValue, "Layer Category", "The frequency at which the settings will be applied")
                                ->Attribute(AZ::Edit::Attributes::EnumValues, &PostFxLayerComponentConfig::BuildLayerCategories)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->DataElement(Edit::UIHandlers::Default, &PostFxLayerComponentConfig::m_priority, "Priority", "The priority this will take over other settings with the same frequency. Lower priority values take precedence.")
                                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &PostFxLayerComponentConfig::GetPriorityLabel)
                                ->Attribute(AZ::Edit::Attributes::Min, 0)
                                ->Attribute(AZ::Edit::Attributes::Max, 20)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &PostFxLayerComponentConfig::m_overrideFactor, "Weight", "How much these settings override previous settings")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PostFxLayerComponentConfig::m_cameraTags, "Select Camera Tags Only", "Limit the PostFx Layer to specific camera entities with the specified tag.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PostFxLayerComponentConfig::m_exclusionTags, "Excluded Camera Tags", "Camera entities containing these tags will not be included.")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorPostFxLayerComponent>()->RequestBus("PostFxLayerRequestBus");

                behaviorContext->ConstantProperty("EditorPostFxLayerComponentTypeId", BehaviorConstant(Uuid(EditorPostFxLayerComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorPostFxLayerComponent::EditorPostFxLayerComponent(const PostFxLayerComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorPostFxLayerComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            m_controller.RebuildCameraEntitiesList();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
