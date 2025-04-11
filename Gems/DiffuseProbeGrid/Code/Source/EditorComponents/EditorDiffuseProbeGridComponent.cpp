/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorDiffuseProbeGridComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/SystemFile.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/Utils/DdsFile.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
AZ_POP_DISABLE_WARNING

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
                    ->Version(3, ConvertToEditorRenderComponentAdapter<1>) // added emissive multiplier
                    ->Field("probeSpacingX", &EditorDiffuseProbeGridComponent::m_probeSpacingX)
                    ->Field("probeSpacingY", &EditorDiffuseProbeGridComponent::m_probeSpacingY)
                    ->Field("probeSpacingZ", &EditorDiffuseProbeGridComponent::m_probeSpacingZ)
                    ->Field("ambientMultiplier", &EditorDiffuseProbeGridComponent::m_ambientMultiplier)
                    ->Field("viewBias", &EditorDiffuseProbeGridComponent::m_viewBias)
                    ->Field("normalBias", &EditorDiffuseProbeGridComponent::m_normalBias)
                    ->Field("numRaysPerProbe", &EditorDiffuseProbeGridComponent::m_numRaysPerProbe)
                    ->Field("scrolling", &EditorDiffuseProbeGridComponent::m_scrolling)
                    ->Field("edgeBlendIbl", &EditorDiffuseProbeGridComponent::m_edgeBlendIbl)
                    ->Field("frameUpdateCount", &EditorDiffuseProbeGridComponent::m_frameUpdateCount)
                    ->Field("transparencyMode", &EditorDiffuseProbeGridComponent::m_transparencyMode)
                    ->Field("emissiveMultiplier", &EditorDiffuseProbeGridComponent::m_emissiveMultiplier)
                    ->Field("editorMode", &EditorDiffuseProbeGridComponent::m_editorMode)
                    ->Field("runtimeMode", &EditorDiffuseProbeGridComponent::m_runtimeMode)
                    ->Field("showVisualization", &EditorDiffuseProbeGridComponent::m_showVisualization)
                    ->Field("showInactiveProbes", &EditorDiffuseProbeGridComponent::m_showInactiveProbes)
                    ->Field("visualizationSphereRadius", &EditorDiffuseProbeGridComponent::m_visualizationSphereRadius)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorDiffuseProbeGridComponent>(
                        "Diffuse Probe Grid", "The DiffuseProbeGrid component generates a grid of diffuse light probes for global illumination")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/diffuse-probe-grid/")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::ModelAsset>::Uuid())
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Probe Spacing")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDiffuseProbeGridComponent::m_probeSpacingX, "X-Axis", "Meters between probes on the X-axis")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Suffix, " meters")
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnProbeSpacingValidateX)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnProbeSpacingChanged)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDiffuseProbeGridComponent::m_probeSpacingY, "Y-Axis", "Meters between probes on the Y-axis")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Suffix, " meters")
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnProbeSpacingValidateY)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnProbeSpacingChanged)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDiffuseProbeGridComponent::m_probeSpacingZ, "Z-Axis", "Meters between probes on the Z-axis")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Suffix, " meters")
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnProbeSpacingValidateZ)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnProbeSpacingChanged)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Grid Settings")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorDiffuseProbeGridComponent::m_ambientMultiplier, "Ambient Multiplier", "Multiplier for the irradiance intensity")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnAmbientMultiplierChanged)
                                ->Attribute(Edit::Attributes::Decimals, 1)
                                ->Attribute(Edit::Attributes::Step, 0.1f)
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
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorDiffuseProbeGridComponent::m_numRaysPerProbe, "Number of Rays Per Probe", "Number of rays cast by each probe to detect lighting in its surroundings")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnNumRaysPerProbeChanged)
                                ->Attribute(AZ::Edit::Attributes::EnumValues, &EditorDiffuseProbeGridComponent::GetNumRaysPerProbeEnumList)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorDiffuseProbeGridComponent::m_scrolling, "Scrolling", "Scrolling causes the grid to move probes on the edges of the volume when it is translated, instead of moving all of the probes.  Use scrolling when the DiffuseProbeGrid is attached to a camera or moving entity.")
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnScrollingChangeValidate)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnScrollingChanged)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorDiffuseProbeGridComponent::m_edgeBlendIbl, "Edge Blend IBL", "Blend the edges of the DiffuseProbeGrid with the Diffuse IBL cubemap.")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnEdgeBlendIblChanged)
                            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &EditorDiffuseProbeGridComponent::m_frameUpdateCount, "Number of Update Frames", "The number of frames to update the complete DiffuseProbeGrid, by updating a subset of the probes each frame.  This will improve the performance of the Real-Time DiffuseProbeGrid update.")
                                ->Attribute(Edit::Attributes::Min, 1)
                                ->Attribute(Edit::Attributes::Max, 10)
                                ->Attribute(AZ::Edit::Attributes::SoftMin, 1)
                                ->Attribute(AZ::Edit::Attributes::SoftMax, 10)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnFrameUpdateCountChanged)
                            ->DataElement(Edit::UIHandlers::ComboBox, &EditorDiffuseProbeGridComponent::m_transparencyMode, "Transparency Mode", "Controls how the DiffuseProbeGrid handles transparent geometry in the Real-Time update, and is a performance/quality tradeoff.  'Full' processes all transparencies found along the probe rays.  'Closest Only' processes only the closest transparency to the probe.  'None' disables transparency handling and treats all geometry as Opaque.")
                                ->EnumAttribute(DiffuseProbeGridTransparencyMode::Full, "Full")
                                ->EnumAttribute(DiffuseProbeGridTransparencyMode::ClosestOnly, "Closest Only")
                                ->EnumAttribute(DiffuseProbeGridTransparencyMode::None, "None")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnTransparencyModeChanged)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorDiffuseProbeGridComponent::m_emissiveMultiplier, "Emissive Multiplier", "Multiplier for the emissive intensity")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnEmissiveMultiplierChanged)
                                ->Attribute(Edit::Attributes::Decimals, 1)
                                ->Attribute(Edit::Attributes::Step, 0.1f)
                                ->Attribute(Edit::Attributes::Min, 0.0f)
                                ->Attribute(Edit::Attributes::Max, 10.0f)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Visualization")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorDiffuseProbeGridComponent::m_showVisualization, "Show Visualization", "Show the probe grid visualization")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnShowVisualizationChanged)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorDiffuseProbeGridComponent::m_showInactiveProbes, "Show Inactive Probes", "Show inactive probes in the probe grid visualization")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnShowInactiveProbesChanged)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorDiffuseProbeGridComponent::m_visualizationSphereRadius, "Visualization Sphere Radius", "Radius of the spheres in the probe grid visualization")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnVisualizationSphereRadiusChanged)
                                ->Attribute(Edit::Attributes::Decimals, 2)
                                ->Attribute(Edit::Attributes::Step, 0.25f)
                                ->Attribute(Edit::Attributes::Min, 0.25f)
                                ->Attribute(Edit::Attributes::Max, 2.0f)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Grid mode")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(Edit::UIHandlers::ComboBox, &EditorDiffuseProbeGridComponent::m_editorMode, "Editor Mode", "Controls whether the editor uses RealTime or Baked diffuse GI. RealTime requires a ray-tracing capable GPU. Auto-Select will fallback to Baked if ray-tracing is not available")
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnModeChangeValidate)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnEditorModeChanged)
                                ->EnumAttribute(DiffuseProbeGridMode::RealTime, "Real Time (Ray-Traced)")
                                ->EnumAttribute(DiffuseProbeGridMode::Baked, "Baked")
                                ->EnumAttribute(DiffuseProbeGridMode::AutoSelect, "Auto Select")
                            ->DataElement(Edit::UIHandlers::ComboBox, &EditorDiffuseProbeGridComponent::m_runtimeMode, "Runtime Mode", "Controls whether the runtime uses RealTime or Baked diffuse GI. RealTime requires a ray-tracing capable GPU. Auto-Select will fallback to Baked if ray-tracing is not available")
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorDiffuseProbeGridComponent::OnModeChangeValidate)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::OnRuntimeModeChanged)
                                ->EnumAttribute(DiffuseProbeGridMode::RealTime, "Real Time (Ray-Traced)")
                                ->EnumAttribute(DiffuseProbeGridMode::Baked, "Baked")
                                ->EnumAttribute(DiffuseProbeGridMode::AutoSelect, "Auto Select")
                        ->EndGroup()
                        ->UIElement(AZ::Edit::UIHandlers::Button, "Bake Textures", "Bake the Diffuse Probe Grid textures to static assets that will be used when the mode is set to Baked")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Bake Textures")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiffuseProbeGridComponent::BakeDiffuseProbeGrid)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorDiffuseProbeGridComponent::GetBakeDiffuseProbeGridVisibilitySetting)
                        ;

                    editContext->Class<DiffuseProbeGridComponentController>(
                        "DiffuseProbeGridComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DiffuseProbeGridComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
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
            AZ::TickBus::Handler::BusConnect();
            AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusConnect();
            m_boxChangedByGridHandler = AZ::Event<bool>::Handler([this]([[maybe_unused]] bool value)
                {
                    this->InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
                });
            m_controller.RegisterBoxChangedByGridHandler(m_boxChangedByGridHandler);

            AZ::u64 entityId = (AZ::u64)GetEntityId();
            m_controller.m_configuration.m_entityId = entityId;
        }

        void EditorDiffuseProbeGridComponent::Deactivate()
        {
            m_editorModeSet = false;

            m_boxChangedByGridHandler.Disconnect();
            AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorDiffuseProbeGridComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (!m_controller.m_featureProcessor)
            {
                return;
            }

            DiffuseProbeGridComponentConfig& configuration = m_controller.m_configuration;

            // set the editor mode, which will override the runtime mode set by the controller
            if (!m_editorModeSet)
            {
                m_controller.m_featureProcessor->SetMode(m_controller.m_handle, configuration.m_editorMode);
                m_editorModeSet = true;
            }

            CheckTextureAssetNotification(configuration.m_bakedIrradianceTextureRelativePath, configuration.m_bakedIrradianceTextureAsset);
            CheckTextureAssetNotification(configuration.m_bakedDistanceTextureRelativePath, configuration.m_bakedDistanceTextureAsset);
            CheckTextureAssetNotification(configuration.m_bakedProbeDataTextureRelativePath, configuration.m_bakedProbeDataTextureAsset);
        }

        void EditorDiffuseProbeGridComponent::CheckTextureAssetNotification(const AZStd::string& relativePath, Data::Asset<RPI::StreamingImageAsset>& configurationAsset)
        {
            Data::Asset<RPI::StreamingImageAsset> textureAsset;
            DiffuseProbeGridTextureNotificationType notificationType = DiffuseProbeGridTextureNotificationType::None;
            if (m_controller.m_featureProcessor->CheckTextureAssetNotification(relativePath + ".streamingimage", textureAsset, notificationType))
            {
                if (notificationType == DiffuseProbeGridTextureNotificationType::Ready)
                {
                    // bake is complete, update configuration with the new baked texture asset
                    AzToolsFramework::ScopedUndoBatch undoBatch("DiffuseProbeGrid Texture Bake");
                    configurationAsset = textureAsset;
                    SetDirty();

                    if (m_controller.m_configuration.m_bakedIrradianceTextureAsset.IsReady() &&
                        m_controller.m_configuration.m_bakedDistanceTextureAsset.IsReady() &&
                        m_controller.m_configuration.m_bakedProbeDataTextureAsset.IsReady())
                    {
                        m_controller.UpdateBakedTextures();
                    }
                }
                else if (notificationType == DiffuseProbeGridTextureNotificationType::Error)
                {
                    QMessageBox::information(
                        QApplication::activeWindow(),
                        "Diffuse Probe Grid",
                        "Diffuse Probe Grid texture failed to bake, please check the Asset Processor for more information.",
                        QMessageBox::Ok);
                }
            }
        }

        AZStd::vector<Edit::EnumConstant<DiffuseProbeGridNumRaysPerProbe>> EditorDiffuseProbeGridComponent::GetNumRaysPerProbeEnumList() const
        {
            AZStd::vector<Edit::EnumConstant<DiffuseProbeGridNumRaysPerProbe>> enumList;

            for (uint32_t index = 0; index < DiffuseProbeGridNumRaysPerProbeArraySize; ++index)
            {
                const DiffuseProbeGridNumRaysPerProbeEntry& entry = DiffuseProbeGridNumRaysPerProbeArray[index];
                enumList.push_back(Edit::EnumConstant<DiffuseProbeGridNumRaysPerProbe>(entry.m_enum, AZStd::to_string(entry.m_rayCount).c_str()));
            }

            return enumList;
        }

        AZ::Aabb EditorDiffuseProbeGridComponent::GetEditorSelectionBoundsViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return m_controller.GetAabb();
        }

        bool EditorDiffuseProbeGridComponent::SupportsEditorRayIntersect()
        {
            return false;
        }

        void EditorDiffuseProbeGridComponent::OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible)
        {
            if ((GetEntityId() == entityId) && !visible)
            {
                m_editorModeSet = false;
            }
        }

        AZ::Outcome<void, AZStd::string> EditorDiffuseProbeGridComponent::OnProbeSpacingValidateX(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            if (!m_controller.m_featureProcessor)
            {
                return AZ::Failure(AZStd::string("This Diffuse Probe Grid entity is hidden, it must be visible in order to change the probe spacing."));
            }

            float newProbeSpacingX = *(reinterpret_cast<float*>(newValue));

            Vector3 newSpacing(newProbeSpacingX, m_probeSpacingY, m_probeSpacingZ);
            if (!m_controller.ValidateProbeSpacing(newSpacing))
            {
                return AZ::Failure(AZStd::string("Probe spacing exceeds max allowable grid size with current extents."));
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> EditorDiffuseProbeGridComponent::OnProbeSpacingValidateY(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            if (!m_controller.m_featureProcessor)
            {
                return AZ::Failure(AZStd::string("This Diffuse Probe Grid entity is hidden, it must be visible in order to change the probe spacing."));
            }

            float newProbeSpacingY = *(reinterpret_cast<float*>(newValue));

            Vector3 newSpacing(m_probeSpacingX, newProbeSpacingY, m_probeSpacingZ);
            if (!m_controller.ValidateProbeSpacing(newSpacing))
            {
                return AZ::Failure(AZStd::string("Probe spacing exceeds max allowable grid size with current extents."));
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> EditorDiffuseProbeGridComponent::OnProbeSpacingValidateZ(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            if (!m_controller.m_featureProcessor)
            {
                return AZ::Failure(AZStd::string("This Diffuse Probe Grid entity is hidden, it must be visible in order to change the probe spacing."));
            }

            float newProbeSpacingZ = *(reinterpret_cast<float*>(newValue));

            Vector3 newSpacing(m_probeSpacingX, m_probeSpacingY, newProbeSpacingZ);
            if (!m_controller.ValidateProbeSpacing(newSpacing))
            {
                return AZ::Failure(AZStd::string("Probe spacing exceeds max allowable grid size with current extents."));
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

        AZ::u32 EditorDiffuseProbeGridComponent::OnNumRaysPerProbeChanged()
        {
            m_controller.SetNumRaysPerProbe(m_numRaysPerProbe);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::Outcome<void, AZStd::string> EditorDiffuseProbeGridComponent::OnScrollingChangeValidate([[maybe_unused]] void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            bool newScrolling = (*(reinterpret_cast<bool*>(newValue)));

            // scrolling requires Real-Time mode
            if (newScrolling && (m_editorMode == DiffuseProbeGridMode::Baked || m_runtimeMode == DiffuseProbeGridMode::Baked))
            {
                return AZ::Failure(AZStd::string("Scrolling requires that the Editor and Runtime modes are both set to Real-Time."));
            }

            return AZ::Success();
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnScrollingChanged()
        {
            m_controller.SetScrolling(m_scrolling);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnEdgeBlendIblChanged()
        {
            m_controller.SetEdgeBlendIbl(m_edgeBlendIbl);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnFrameUpdateCountChanged()
        {
            m_controller.SetFrameUpdateCount(m_frameUpdateCount);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnTransparencyModeChanged()
        {
            m_controller.SetTransparencyMode(m_transparencyMode);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnEmissiveMultiplierChanged()
        {
            m_controller.SetEmissiveMultiplier(m_emissiveMultiplier);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnEditorModeChanged()
        {
            // this will update the configuration and also change the DiffuseProbeGrid mode
            m_controller.SetEditorMode(m_editorMode);
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnRuntimeModeChanged()
        {
            // this will only update the configuration
            m_controller.SetRuntimeMode(m_runtimeMode);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnShowVisualizationChanged()
        {
            m_controller.SetVisualizationEnabled(m_showVisualization);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnShowInactiveProbesChanged()
        {
            m_controller.SetVisualizationShowInactiveProbes(m_showInactiveProbes);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::OnVisualizationSphereRadiusChanged()
        {
            m_controller.SetVisualizationSphereRadius(m_visualizationSphereRadius);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::Outcome<void, AZStd::string> EditorDiffuseProbeGridComponent::OnModeChangeValidate([[maybe_unused]] void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            DiffuseProbeGridMode newMode = (*(reinterpret_cast<DiffuseProbeGridMode*>(newValue)));

            if (newMode == DiffuseProbeGridMode::Baked || newMode == DiffuseProbeGridMode::AutoSelect)
            {
                if (!m_controller.m_configuration.m_bakedIrradianceTextureAsset.GetId().IsValid() ||
                    !m_controller.m_configuration.m_bakedDistanceTextureAsset.GetId().IsValid() ||
                    !m_controller.m_configuration.m_bakedProbeDataTextureAsset.GetId().IsValid())
                {
                    return AZ::Failure(AZStd::string("Please bake textures before changing the Diffuse Probe Grid to Baked or Auto-Select mode."));
                }

                // scrolling requires Real-Time mode
                if (m_scrolling)
                {
                    return AZ::Failure(AZStd::string("Scrolling requires that the Editor and Runtime modes are both set to Real-Time."));
                }
            }

            return AZ::Success();
        }

        AZ::u32 EditorDiffuseProbeGridComponent::GetBakeDiffuseProbeGridVisibilitySetting()
        {
            // the Bake button is visible only when the editor mode is set to RealTime
            return m_editorMode == DiffuseProbeGridMode::RealTime ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::u32 EditorDiffuseProbeGridComponent::BakeDiffuseProbeGrid()
        {
            if (m_bakeInProgress)
            {
                return AZ::Edit::PropertyRefreshLevels::None;
            }

            // retrieve entity visibility
            bool isHidden = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                isHidden,
                GetEntityId(),
                &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsHidden);

            // the entity must be visible in order to bake
            if (isHidden)
            {
                QMessageBox::information(
                    QApplication::activeWindow(),
                    "Diffuse Probe Grid",
                    "This Diffuse Probe Grid entity is hidden, it must be visible in order to bake textures.",
                    QMessageBox::Ok);

                return AZ::Edit::PropertyRefreshLevels::None;
            }

            if (!m_controller.CanBakeTextures())
            {
                QMessageBox::information(
                    QApplication::activeWindow(),
                    "Diffuse Probe Grid",
                    "Can't bake the textures. Diffuse probe calculations require GPU raytracing support",
                    QMessageBox::Ok);

                return AZ::Edit::PropertyRefreshLevels::None;
            }

            DiffuseProbeGridComponentConfig& configuration = m_controller.m_configuration;

            // retrieve the source image paths from the configuration
            // Note: we need to make sure to use the same source image for each bake
            AZStd::string irradianceTextureRelativePath = ValidateOrCreateNewTexturePath(configuration.m_bakedIrradianceTextureRelativePath, DiffuseProbeGridIrradianceFileName);
            AZStd::string distanceTextureRelativePath = ValidateOrCreateNewTexturePath(configuration.m_bakedDistanceTextureRelativePath, DiffuseProbeGridDistanceFileName);
            AZStd::string probeDataTextureRelativePath = ValidateOrCreateNewTexturePath(configuration.m_bakedProbeDataTextureRelativePath, DiffuseProbeGridProbeDataFileName);

            // create the full paths
            char projectPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", projectPath, AZ_MAX_PATH_LEN);

            AZStd::string irradianceTextureFullPath;
            AzFramework::StringFunc::Path::Join(projectPath, irradianceTextureRelativePath.c_str(), irradianceTextureFullPath, true, true);
            AZStd::string distanceTextureFullPath;
            AzFramework::StringFunc::Path::Join(projectPath, distanceTextureRelativePath.c_str(), distanceTextureFullPath, true, true);
            AZStd::string probeDataTextureFullPath;
            AzFramework::StringFunc::Path::Join(projectPath, probeDataTextureRelativePath.c_str(), probeDataTextureFullPath, true, true);

            // make sure the folder is created
            AZStd::string diffuseProbeGridFolder;
            AzFramework::StringFunc::Path::GetFolderPath(irradianceTextureFullPath.data(), diffuseProbeGridFolder);
            AZ::IO::SystemFile::CreateDir(diffuseProbeGridFolder.c_str());

            // check out the files in source control
            CheckoutSourceTextureFile(irradianceTextureFullPath);
            CheckoutSourceTextureFile(distanceTextureFullPath);
            CheckoutSourceTextureFile(probeDataTextureFullPath);

            // update the configuration
            AzToolsFramework::ScopedUndoBatch undoBatch("DiffuseProbeGrid bake");
            configuration.m_bakedIrradianceTextureRelativePath = irradianceTextureRelativePath;
            configuration.m_bakedDistanceTextureRelativePath = distanceTextureRelativePath;
            configuration.m_bakedProbeDataTextureRelativePath = probeDataTextureRelativePath;
            SetDirty();

            // callback for the texture readback
            DiffuseProbeGridBakeTexturesCallback bakeTexturesCallback = [=](
                DiffuseProbeGridTexture irradianceTexture,
                DiffuseProbeGridTexture distanceTexture,
                DiffuseProbeGridTexture probeDataTexture)
            {
                // irradiance
                {
                    AZ::DdsFile::DdsFileData fileData = { irradianceTexture.m_size, irradianceTexture.m_format, irradianceTexture.m_data.get() };
                    [[maybe_unused]] const auto outcome = AZ::DdsFile::WriteFile(irradianceTextureFullPath, fileData);
                    AZ_Assert(outcome.IsSuccess(), "Failed to write Irradiance texture .dds file [%s]", irradianceTextureFullPath.c_str());
                }

                // distance
                {
                    AZ::DdsFile::DdsFileData fileData = { distanceTexture.m_size, distanceTexture.m_format, distanceTexture.m_data.get() };
                    [[maybe_unused]] const auto outcome = AZ::DdsFile::WriteFile(distanceTextureFullPath, fileData);
                    AZ_Assert(outcome.IsSuccess(), "Failed to write Distance texture .dds file [%s]", distanceTextureFullPath.c_str());
                }

                // probe data
                {
                    AZ::DdsFile::DdsFileData fileData = { probeDataTexture.m_size, probeDataTexture.m_format, probeDataTexture.m_data.get() };
                    [[maybe_unused]] const auto outcome = AZ::DdsFile::WriteFile(probeDataTextureFullPath, fileData);
                    AZ_Assert(outcome.IsSuccess(), "Failed to write ProbeData texture .dds file [%s]", probeDataTextureFullPath.c_str());
                }

                m_bakeInProgress = false;
            };

            m_bakeInProgress = true;
            m_controller.BakeTextures(bakeTexturesCallback);

            QProgressDialog bakeDialog;
            bakeDialog.setWindowFlags(bakeDialog.windowFlags() & ~Qt::WindowCloseButtonHint);
            bakeDialog.setLabelText(QObject::tr("Baking Diffuse Probe Grid..."));
            bakeDialog.setWindowModality(Qt::WindowModal);
            bakeDialog.setMaximumSize(QSize(256, 96));
            bakeDialog.setMaximum(0);
            bakeDialog.setMinimumDuration(0);
            bakeDialog.setAutoClose(false);
            bakeDialog.setCancelButton(nullptr);
            bakeDialog.show();

            while (m_bakeInProgress)
            {
                if (bakeDialog.wasCanceled())
                {
                    m_bakeInProgress = false;
                    break;
                }

                QApplication::processEvents();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
            }

            QMessageBox::information(
                QApplication::activeWindow(),
                "Diffuse Probe Grid",
                "Successfully baked Diffuse Probe Grid textures.",
                QMessageBox::Ok);

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZStd::string EditorDiffuseProbeGridComponent::ValidateOrCreateNewTexturePath(const AZStd::string& configurationRelativePath, const char* fileSuffix)
        {
            AZStd::string relativePath = configurationRelativePath;
            AZStd::string fullPath;

            char projectPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", projectPath, AZ_MAX_PATH_LEN);

            if (!relativePath.empty())
            {
                // test to see if the texture file is actually there, if it was removed we need to
                // generate a new filename, otherwise it will cause an error in the asset system
                AzFramework::StringFunc::Path::Join(projectPath, configurationRelativePath.c_str(), fullPath, true, true);

                if (!AZ::IO::FileIOBase::GetInstance()->Exists(fullPath.c_str()))
                {
                    // file does not exist, clear the relative path so we generate a new name
                    relativePath.clear();
                }
            }
                         
            // build a new image path if necessary
            if (relativePath.empty())
            {
                // the file name is a combination of the entity name, a UUID, and the filemask
                Entity* entity = GetEntity();
                AZ_Assert(entity, "DiffuseProbeGrid entity is null");

                AZ::Uuid uuid = AZ::Uuid::CreateRandom();
                AZStd::string uuidString;
                uuid.ToString(uuidString);

                relativePath = "DiffuseProbeGrids/" + entity->GetName() + uuidString + fileSuffix;

                // replace any invalid filename characters
                auto invalidCharacters = [](char letter)
                {
                    return
                        letter == ':' || letter == '"' || letter == '\'' ||
                        letter == '{' || letter == '}' ||
                        letter == '<' || letter == '>';
                };
                AZStd::replace_if(relativePath.begin(), relativePath.end(), invalidCharacters, '_');
            }

            return relativePath;
        }

        void EditorDiffuseProbeGridComponent::CheckoutSourceTextureFile(const AZStd::string& fullPath)
        {
            bool checkedOutSuccessfully = false;
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            ApplicationBus::BroadcastResult(
                checkedOutSuccessfully,
                &ApplicationBus::Events::RequestEditForFileBlocking,
                fullPath.c_str(),
                "Checking out for edit...",
                ApplicationBus::Events::RequestEditProgressCallback());
        }
    } // namespace Render
} // namespace AZ
