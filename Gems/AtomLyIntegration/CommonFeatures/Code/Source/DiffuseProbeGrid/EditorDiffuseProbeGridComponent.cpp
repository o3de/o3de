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

#include <DiffuseProbeGrid/EditorDiffuseProbeGridComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Component/Entity.h>

namespace AZ
{
    namespace Render
    {
        void EditorDiffuseProbeGridComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDiffuseProbeGridComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>)
                    ->Field("probeSpacingX", &EditorDiffuseProbeGridComponent::m_probeSpacingX)
                    ->Field("probeSpacingY", &EditorDiffuseProbeGridComponent::m_probeSpacingY)
                    ->Field("probeSpacingZ", &EditorDiffuseProbeGridComponent::m_probeSpacingZ)
                    ->Field("ambientMultiplier", &EditorDiffuseProbeGridComponent::m_ambientMultiplier)
                    ->Field("viewBias", &EditorDiffuseProbeGridComponent::m_viewBias)
                    ->Field("normalBias", &EditorDiffuseProbeGridComponent::m_normalBias)
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDiffuseProbeGridComponent>(
                        "Diffuse Probe Grid", "The DiffuseProbeGrid component generates a grid of diffuse light probes for global illumination")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::ModelAsset>::Uuid())
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Probe Spacing")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDiffuseProbeGridComponent::m_probeSpacingX, "X", "Probe spacing on the X-axis")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnProbeSpacingValidateX)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnProbeSpacingChanged)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDiffuseProbeGridComponent::m_probeSpacingY, "Y", "Probe spacing on the Y-axis")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnProbeSpacingValidateY)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnProbeSpacingChanged)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDiffuseProbeGridComponent::m_probeSpacingZ, "Z", "Probe spacing on the Z-axis")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnProbeSpacingValidateZ)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnProbeSpacingChanged)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Grid Settings")
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorDiffuseProbeGridComponent::m_ambientMultiplier, "Ambient Multiplier", "Multiplier for the irradiance intensity")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnAmbientMultiplierChanged)
                                ->Attribute(Edit::Attributes::Decimals, 0)
                                ->Attribute(Edit::Attributes::Step, 1.0f)
                                ->Attribute(Edit::Attributes::Min, 0.0f)
                                ->Attribute(Edit::Attributes::Max, 10.0f)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorDiffuseProbeGridComponent::m_viewBias, "View Bias", "View bias adjustment")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnViewBiasChanged)
                                ->Attribute(Edit::Attributes::Decimals, 2)
                                ->Attribute(Edit::Attributes::Step, 0.1f)
                                ->Attribute(Edit::Attributes::Min, 0.0f)
                                ->Attribute(Edit::Attributes::Max, 1.0f)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorDiffuseProbeGridComponent::m_normalBias, "Normal Bias", "Normal bias adjustment")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnNormalBiasChanged)
                                ->Attribute(Edit::Attributes::Decimals, 2)
                                ->Attribute(Edit::Attributes::Step, 0.1f)
                                ->Attribute(Edit::Attributes::Min, 0.0f)
                                ->Attribute(Edit::Attributes::Max, 1.0f)
                        ;

                    editContext->Class<DiffuseProbeGridComponentController>(
                        "DiffuseProbeGridComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DiffuseProbeGridComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<DiffuseProbeGridComponentConfig>(
                        "DiffuseProbeGridComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorDiffuseProbeGridComponentTypeId", BehaviorConstant(Uuid(EditorDiffuseProbeGridComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorDiffuseProbeGridComponent::EditorDiffuseProbeGridComponent()
        {
        }

        EditorDiffuseProbeGridComponent::EditorDiffuseProbeGridComponent(const DiffuseProbeGridComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorDiffuseProbeGridComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
        }

        void EditorDiffuseProbeGridComponent::Deactivate()
        {
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        AZ::Aabb EditorDiffuseProbeGridComponent::GetEditorSelectionBoundsViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return m_controller.GetAabb();
        }

        bool EditorDiffuseProbeGridComponent::SupportsEditorRayIntersect()
        {
            return false;
        }

        AZ::Outcome<void, AZStd::string> EditorDiffuseProbeGridComponent::OnProbeSpacingValidateX(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            if (!m_controller.m_featureProcessor)
            {
                return AZ::Failure(AZStd::string("Unable to adjust probe spacing, please try again"));
            }

            float newProbeSpacingX = *(reinterpret_cast<float*>(newValue));

            Vector3 newSpacing(newProbeSpacingX, m_probeSpacingY, m_probeSpacingZ);
            if (!m_controller.ValidateProbeSpacing(newSpacing))
            {
                return AZ::Failure(AZStd::string("Probe spacing exceeds max allowable grid size with current extents"));
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> EditorDiffuseProbeGridComponent::OnProbeSpacingValidateY(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            if (!m_controller.m_featureProcessor)
            {
                return AZ::Failure(AZStd::string("Unable to adjust probe spacing, please try again"));
            }

            float newProbeSpacingY = *(reinterpret_cast<float*>(newValue));

            Vector3 newSpacing(m_probeSpacingX, newProbeSpacingY, m_probeSpacingZ);
            if (!m_controller.ValidateProbeSpacing(newSpacing))
            {
                return AZ::Failure(AZStd::string("Probe spacing exceeds max allowable grid size with current extents"));
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> EditorDiffuseProbeGridComponent::OnProbeSpacingValidateZ(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            if (!m_controller.m_featureProcessor)
            {
                return AZ::Failure(AZStd::string("Unable to adjust probe spacing, please try again"));
            }

            float newProbeSpacingZ = *(reinterpret_cast<float*>(newValue));

            Vector3 newSpacing(m_probeSpacingX, m_probeSpacingY, newProbeSpacingZ);
            if (!m_controller.ValidateProbeSpacing(newSpacing))
            {
                return AZ::Failure(AZStd::string("Probe spacing exceeds max allowable grid size with current extents"));
            }

            return AZ::Success();
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnProbeSpacingChanged()
        {
            AZ::Vector3 probeSpacing(m_probeSpacingX, m_probeSpacingY, m_probeSpacingZ);
            m_controller.SetProbeSpacing(probeSpacing);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnAmbientMultiplierChanged()
        {
            m_controller.SetAmbientMultiplier(m_ambientMultiplier);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnViewBiasChanged()
        {
            m_controller.SetViewBias(m_viewBias);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnNormalBiasChanged()
        {
            m_controller.SetNormalBias(m_normalBias);
            return AZ::Edit::PropertyRefreshLevels::None;
        }
    } // namespace Render
} // namespace AZ
