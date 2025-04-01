/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ReflectionProbe/EditorReflectionProbeComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/IO/SystemFile.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        void EditorReflectionProbeComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorReflectionProbeComponent, BaseClass>()
                    ->Version(2, ConvertToEditorRenderComponentAdapter<1>)
                    ->Field("useBakedCubemap", &EditorReflectionProbeComponent::m_useBakedCubemap)
                    ->Field("bakedCubeMapQualityLevel", &EditorReflectionProbeComponent::m_bakedCubeMapQualityLevel)
                    ->Field("bakedCubeMapRelativePath", &EditorReflectionProbeComponent::m_bakedCubeMapRelativePath)
                    ->Field("authoredCubeMapAsset", &EditorReflectionProbeComponent::m_authoredCubeMapAsset)
                    ->Field("bakeExposure", &EditorReflectionProbeComponent::m_bakeExposure)
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorReflectionProbeComponent>(
                        "Reflection Probe", "The ReflectionProbe component captures an IBL specular reflection at a specific position in the level")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Lighting")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/reflection-probe/")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::ModelAsset>::Uuid())
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Cubemap Bake")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->UIElement(AZ::Edit::UIHandlers::Button, "Bake Reflection Probe", "Bake Reflection Probe")
                                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                                ->Attribute(AZ::Edit::Attributes::ButtonText, "Bake Reflection Probe")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorReflectionProbeComponent::BakeReflectionProbe)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorReflectionProbeComponent::m_bakeExposure, "Bake Exposure", "Exposure to use when baking the cubemap")
                                ->Attribute(AZ::Edit::Attributes::SoftMin, -16.0f)
                                ->Attribute(AZ::Edit::Attributes::SoftMax, 16.0f)
                                ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                                ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorReflectionProbeComponent::OnBakeExposureChanged)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Cubemap")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorReflectionProbeComponent::m_useBakedCubemap, "Use Baked Cubemap", "Selects between a cubemap that captures the environment at location in the scene or a preauthored cubemap")
                                ->Attribute(AZ::Edit::Attributes::ChangeValidate, &EditorReflectionProbeComponent::OnUseBakedCubemapValidate)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorReflectionProbeComponent::OnUseBakedCubemapChanged)
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorReflectionProbeComponent::m_bakedCubeMapQualityLevel, "Baked Cubemap Quality", "Resolution of the baked cubemap")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting)
                                ->EnumAttribute(CubeMapSpecularQualityLevel::VeryLow, "Very Low")
                                ->EnumAttribute(CubeMapSpecularQualityLevel::Low, "Low")
                                ->EnumAttribute(CubeMapSpecularQualityLevel::Medium, "Medium")
                                ->EnumAttribute(CubeMapSpecularQualityLevel::High, "High")
                                ->EnumAttribute(CubeMapSpecularQualityLevel::VeryHigh, "Very High")
                            ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorReflectionProbeComponent::m_bakedCubeMapRelativePath, "Baked Cubemap Path", "Baked Cubemap Path")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorReflectionProbeComponent::m_authoredCubeMapAsset, "Cubemap file", "Authored Cubemap file")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorReflectionProbeComponent::GetAuthoredCubemapVisibilitySetting)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorReflectionProbeComponent::OnAuthoredCubemapChanged)
                        ;

                    editContext->Class<ReflectionProbeComponentController>(
                        "ReflectionProbeComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ReflectionProbeComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<ReflectionProbeComponentConfig>(
                        "ReflectionProbeComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Inner Extents")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ReflectionProbeComponentConfig::m_innerHeight, "Height", "Height of the reflection probe inner volume")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ReflectionProbeComponentConfig::m_innerLength, "Length", "Length of the reflection probe inner volume")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &ReflectionProbeComponentConfig::m_innerWidth, "Width", "Width of the reflection probe inner volume")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Settings")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ReflectionProbeComponentConfig::m_useParallaxCorrection, "Parallax Correction", "Correct the reflection to adjust for the offset from the capture position")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ReflectionProbeComponentConfig::m_showVisualization, "Show Visualization", "Show the reflection probe visualization sphere")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &ReflectionProbeComponentConfig::m_renderExposure, "Exposure", "Exposure to use when rendering meshes with the cubemap")
                                ->Attribute(AZ::Edit::Attributes::SoftMin, -5.0f)
                                ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                                ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                                ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorReflectionProbeBus>("EditorReflectionProbeBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("BakeReflectionProbe", &EditorReflectionProbeInterface::BakeReflectionProbe)
                    ;
                
                behaviorContext->ConstantProperty("EditorReflectionProbeComponentTypeId", BehaviorConstant(Uuid(EditorReflectionProbeComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorReflectionProbeComponent::EditorReflectionProbeComponent(const ReflectionProbeComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorReflectionProbeComponent::Activate()
        {
            BaseClass::Activate();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
            EditorReflectionProbeBus::Handler::BusConnect(GetEntityId());
            AZ::TickBus::Handler::BusConnect();

            ReflectionProbeComponentConfig& configuration = m_controller.m_configuration;

            // update UI cubemap path display
            m_bakedCubeMapRelativePath = configuration.m_bakedCubeMapRelativePath;

            AZ::u64 entityId = (AZ::u64)GetEntityId();
            configuration.m_entityId = entityId;
            AZ::EntityComponentIdPair entityComponentId = AZ::EntityComponentIdPair(GetEntityId(), GetId());

            m_innerExtentsChangedHandler = AZ::Event<bool>::Handler([entityComponentId]([[maybe_unused]] bool value)
                {
                    AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplayForComponent,
                        entityComponentId,
                        AzToolsFramework::Refresh_Values);
                });
            m_controller.RegisterInnerExtentsChangedHandler(m_innerExtentsChangedHandler);
        }

        void EditorReflectionProbeComponent::Deactivate()
        {
            m_innerExtentsChangedHandler.Disconnect();
            EditorReflectionProbeBus::Handler::BusDisconnect(GetEntityId());
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorReflectionProbeComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (!m_controller.m_featureProcessor)
            {
                return;
            }

            if (m_controller.m_configuration.m_useBakedCubemap)
            {
                AZStd::string cubeMapRelativePath = m_controller.m_configuration.m_bakedCubeMapRelativePath + ".streamingimage";
                Data::Asset<RPI::StreamingImageAsset> cubeMapAsset;
                CubeMapAssetNotificationType notificationType = CubeMapAssetNotificationType::None;
                if (m_controller.m_featureProcessor->CheckCubeMapAssetNotification(cubeMapRelativePath, cubeMapAsset, notificationType))
                {
                    // a cubemap bake is in progress for this entity component
                    if (notificationType == CubeMapAssetNotificationType::Ready)
                    {
                        // bake is complete, update configuration with the new baked cubemap asset
                        AzToolsFramework::ScopedUndoBatch undoBatch("ReflectionProbe Bake");
                        m_controller.m_configuration.m_bakedCubeMapAsset = cubeMapAsset;
                        SetDirty();

                        // refresh the currently rendered cubemap
                        m_controller.UpdateCubeMap();

                        // update the UI
                        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
                    }
                    else if (notificationType == CubeMapAssetNotificationType::Error)
                    {
                        // cubemap bake failed
                        QMessageBox::information(
                            QApplication::activeWindow(),
                            "Reflection Probe",
                            "Reflection Probe cubemap failed to bake, please check the Asset Processor for more information.",
                            QMessageBox::Ok);

                        // clear relative path, this will allow the user to retry
                        m_controller.m_configuration.m_bakedCubeMapRelativePath.clear();

                        // update the UI
                        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
                    }
                }
            }
        }

        void EditorReflectionProbeComponent::DisplayEntityViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
        {
            // only draw the bounds if selected
            if (!IsSelected())
            {
                return;
            }

            const AZ::Vector3 translationOffset =
                m_controller.m_shapeBus ? m_controller.m_shapeBus->GetTranslationOffset() : AZ::Vector3::CreateZero();

            AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            if (m_controller.m_boxShapeInterface && m_controller.m_boxShapeInterface->IsTypeAxisAligned())
            {
                worldTransform.SetRotation(AZ::Quaternion::CreateIdentity());
            }
            AZ::Quaternion rotationQuaternion = worldTransform.GetRotation();
            AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(rotationQuaternion);
            const AZ::Vector3 position = worldTransform.TransformPoint(translationOffset);

            float scale = worldTransform.GetUniformScale();

            // draw AABB at probe position using the inner dimensions
            Color color(0.0f, 0.0f, 1.0f, 1.0f);
            debugDisplay.SetColor(color);

            ReflectionProbeComponentConfig& configuration = m_controller.m_configuration;
            AZ::Vector3 innerExtents(configuration.m_innerWidth, configuration.m_innerLength, configuration.m_innerHeight);
            innerExtents *= scale;

            debugDisplay.DrawWireOBB(position, rotationMatrix.GetBasisX(), rotationMatrix.GetBasisY(), rotationMatrix.GetBasisZ(), innerExtents / 2.0f);
        }

        AZ::Aabb EditorReflectionProbeComponent::GetEditorSelectionBoundsViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return m_controller.GetWorldBounds();
        }

        bool EditorReflectionProbeComponent::SupportsEditorRayIntersect()
        {
            return false;
        }

        AZ::Outcome<void, AZStd::string> EditorReflectionProbeComponent::OnUseBakedCubemapValidate([[maybe_unused]] void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
        {
            if (!m_controller.m_featureProcessor)
            {
                return AZ::Failure(AZStd::string("This Reflection Probe entity is hidden, it must be visible in order to change the cubemap type."));
            }

            return AZ::Success();
        }

        AZ::u32 EditorReflectionProbeComponent::OnUseBakedCubemapChanged()
        {
            // save setting to the configuration
            m_controller.m_configuration.m_useBakedCubemap = m_useBakedCubemap;

            // refresh currently displayed cubemap
            m_controller.UpdateCubeMap();

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 EditorReflectionProbeComponent::OnAuthoredCubemapChanged()
        {
            // save the selected authored asset to the configuration
            m_controller.m_configuration.m_authoredCubeMapAsset = m_authoredCubeMapAsset;

            // refresh currently displayed cubemap
            m_controller.UpdateCubeMap();

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorReflectionProbeComponent::OnBakeExposureChanged()
        {
            m_controller.SetBakeExposure(m_bakeExposure);

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorReflectionProbeComponent::GetBakedCubemapVisibilitySetting()
        {
            // controls specific to baked cubemaps call this to determine their visibility
            // they are visible when the mode is set to baked, otherwise hidden
            return m_useBakedCubemap ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::u32 EditorReflectionProbeComponent::GetAuthoredCubemapVisibilitySetting()
        {
            // controls specific to authored cubemaps call this to determine their visibility
            // they are hidden when the mode is set to baked, otherwise visible
            return m_useBakedCubemap ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
        }

        AZ::u32 EditorReflectionProbeComponent::BakeReflectionProbe()
        {
            ReflectionProbeComponentConfig& configuration = m_controller.m_configuration;

            // if the quality level changed we need to generate a new filename
            if (configuration.m_bakedCubeMapQualityLevel != m_bakedCubeMapQualityLevel)
            {
                configuration.m_bakedCubeMapRelativePath.clear();
            }

            AzToolsFramework::ScopedUndoBatch undoBatch("ReflectionProbe Bake");

            AZ::u32 result = RenderCubeMap(
                [&](RenderCubeMapCallback callback, AZStd::string& relativePath) { m_controller.BakeReflectionProbe(callback, relativePath); },
                "Baking Reflection Probe...",
                GetEntity(),
                "ReflectionProbes",
                configuration.m_bakedCubeMapRelativePath,
                CubeMapCaptureType::Specular,
                m_bakedCubeMapQualityLevel);

            // update quality level
            m_controller.m_configuration.m_bakedCubeMapQualityLevel = m_bakedCubeMapQualityLevel;

            // update UI cubemap path display
            m_bakedCubeMapRelativePath = configuration.m_bakedCubeMapRelativePath;

            SetDirty();

            return result;
        }
    } // namespace Render
} // namespace AZ
