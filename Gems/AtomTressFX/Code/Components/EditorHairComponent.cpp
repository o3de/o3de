/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined (ATOMTRESSFX_EDITOR)

#include <AzCore/RTTI/BehaviorContext.h>
#include <Components/EditorHairComponent.h>
#include <Rendering/HairRenderObject.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            void EditorHairComponent::Reflect(AZ::ReflectContext* context)
            {
                BaseClass::Reflect(context);

                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<EditorHairComponent, BaseClass>()
                        ->Version(1);

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<EditorHairComponent>(
                            "Atom Hair", "Controls Hair Properties")
                            ->ClassElement(Edit::ClassElements::EditorData, "")
                                ->Attribute(Edit::Attributes::Category, "Atom")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                                ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                                ->Attribute(Edit::Attributes::AutoExpand, true)
                                ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/gems/reference/rendering/amd/atom-tressfx/")
                            ;
                        
                        editContext->Class<HairComponentController>(
                            "HairComponentController", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairComponentController::m_configuration, "Configuration", "")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ;

                        editContext->Class<HairComponentConfig>(
                            "HairComponentConfig", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->DataElement(
                                AZ::Edit::UIHandlers::Default, &HairComponentConfig::m_hairAsset, "Hair Asset",
                                "TressFX asset to be assigned to this entity.")
                            ->DataElement(
                                AZ::Edit::UIHandlers::Default, &HairComponentConfig::m_simulationSettings, "TressFX Sim Settings",
                                "TressFX simulation settings to be applied on this entity.")
                            ->DataElement(
                                AZ::Edit::UIHandlers::Default, &HairComponentConfig::m_renderingSettings, "TressFX Render Settings",
                                "TressFX rendering settings to be applied on this entity.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &HairComponentConfig::m_hairGlobalSettings)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &HairComponentConfig::OnHairGlobalSettingsChanged)
                            ;
                    }
                }

                if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
                {
                    behaviorContext->Class<EditorHairComponent>()->RequestBus("HairRequestsBus");

                    behaviorContext->ConstantProperty("EditorHairComponentTypeId", BehaviorConstant(Uuid(Hair::EditorHairComponentTypeId)))
                        ->Attribute(AZ::Script::Attributes::Module, "render")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
                }
            }

            EditorHairComponent::EditorHairComponent(const HairComponentConfig& config)
                : BaseClass(config)
            {
                m_prevHairAssetId = config.m_hairAsset.GetId();
            }

            void EditorHairComponent::Activate()
            {
                BaseClass::Activate();
                AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            }

            void EditorHairComponent::Deactivate()
            {
                BaseClass::Deactivate();
                AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            }

            u32 EditorHairComponent::OnConfigurationChanged()
            {
                // Since any of the hair config and hair asset change will trigger this call, we use the prev loaded hair assetId
                // to check which config actually got changed.
                // This is because an asset change is a heavy operation and we don't want to trigger that when it's not needed.
                if (m_prevHairAssetId == m_controller.GetConfiguration().m_hairAsset.GetId())
                {
                    m_controller.OnHairConfigChanged();
                }
                else
                {
                    m_controller.OnHairAssetChanged();
                    m_prevHairAssetId = m_controller.GetConfiguration().m_hairAsset.GetId();
                }
                return Edit::PropertyRefreshLevels::AttributesAndValues;
            }

            void EditorHairComponent::DisplayEntityViewport(
                [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
            {
                // Only render debug information when selected.
                if (!IsSelected())
                {
                    return;
                }

                // Only render debug information after render object got created.
                if (!m_controller.m_renderObject)
                {
                    return;
                }

                float x = 40.0;
                float y = 20.0f; 
                float size = 1.00f;
                bool center = false;

                AZStd::string debugString = AZStd::string::format(
                    "Hair component stats:\n"
                    "    Total number of hairs: %d\n"
                    "    Total number of guide hairs: %d\n"
                    "    Amount of follow hair per guide hair: %d\n",
                    m_controller.m_renderObject->GetNumTotalHairStrands(),
                    m_controller.m_renderObject->GetNumGuideHairs(),
                    m_controller.m_renderObject->GetNumFollowHairsPerGuideHair());

                debugDisplay.Draw2dTextLabel(x, y, size, debugString.c_str(), center);
            }
        } // namespace Hair
    } // namespace Render
} // namespace AZ

#endif // ATOMTRESSFX_EDITOR
