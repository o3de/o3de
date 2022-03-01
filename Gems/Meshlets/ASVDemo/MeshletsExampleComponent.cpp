/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MeshletsExampleComponent.h>

#include <Atom/Component/DebugCamera/ArcBallControllerComponent.h>
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>

#include <Atom/RHI/Device.h>
#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/View.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/sort.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#include <SampleComponentManager.h>
#include <SampleComponentConfig.h>
#include <EntityUtilityFunctions.h>

#include <Automation/ScriptableImGui.h>
#include <Automation/ScriptRunnerBus.h>

#include <RHI/BasicRHIComponent.h>


namespace AtomSampleViewer
{
    const char* MeshletsExampleComponent::CameraControllerNameTable[CameraControllerCount] =
    {
        "ArcBall",
        "NoClip"
    };

    void MeshletsExampleComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MeshletsExampleComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    MeshletsExampleComponent::MeshletsExampleComponent()
        : m_materialBrowser("@user@/MeshExampleComponent/material_browser.xml")
        , m_modelBrowser("@user@/MeshExampleComponent/model_browser.xml")
        , m_imguiSidebar("@user@/MeshExampleComponent/sidebar.xml")
    {
        m_changedHandler = AZ::Render::MeshFeatureProcessorInterface::ModelChangedEvent::Handler
        {
            [&](AZ::Data::Instance<AZ::RPI::Model> model)
            {
                ScriptRunnerRequestBus::Broadcast(&ScriptRunnerRequests::ResumeScript);

                // This handler will be connected to the feature processor so that when the model is updated, the camera
                // controller will reset. This ensures the camera is a reasonable distance from the model when it resizes.
                ResetCameraController();

                UpdateGroundPlane();
            }
        };
    }

    void MeshletsExampleComponent::DefaultWindowCreated()
    {
        AZ::Render::Bootstrap::DefaultWindowBus::BroadcastResult(m_windowContext, &AZ::Render::Bootstrap::DefaultWindowBus::Events::GetDefaultWindowContext);
    }

    void MeshletsExampleComponent::CreateLowEndPipeline()
    {
        AZ::RPI::RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_mainViewTagName = "MainCamera";
        pipelineDesc.m_name = "LowEndPipeline";
        pipelineDesc.m_rootPassTemplate = "LowEndPipelineTemplate";
        pipelineDesc.m_renderSettings.m_multisampleState.m_samples = 4;

        m_lowEndPipeline = AZ::RPI::RenderPipeline::CreateRenderPipelineForWindow(pipelineDesc, *m_windowContext);
    }

    void MeshletsExampleComponent::DestroyLowEndPipeline()
    {
        m_lowEndPipeline = nullptr;
    }

    void MeshletsExampleComponent::ActivateLowEndPipeline()
    {
        m_originalPipeline = m_scene->GetDefaultRenderPipeline();
        m_scene->AddRenderPipeline(m_lowEndPipeline);
        m_lowEndPipeline->SetDefaultView(m_originalPipeline->GetDefaultView());
        m_scene->RemoveRenderPipeline(m_originalPipeline->GetId());

        m_imguiScope = AZ::Render::ImGuiActiveContextScope::FromPass({ m_lowEndPipeline->GetId().GetCStr(), "ImGuiPass" });
    }

    void MeshletsExampleComponent::DeactivateLowEndPipeline()
    {
        m_imguiScope = {}; // restores previous ImGui context.

        m_scene->AddRenderPipeline(m_originalPipeline);
        m_scene->RemoveRenderPipeline(m_lowEndPipeline->GetId());
    }

    void MeshletsExampleComponent::Activate()
    {
        UseArcBallCameraController();

        m_materialBrowser.SetFilter([this](const AZ::Data::AssetInfo& assetInfo)
        {
            if (!AzFramework::StringFunc::Path::IsExtension(assetInfo.m_relativePath.c_str(), "azmaterial"))
            {
                return false;
            }
            if (m_showModelMaterials)
            {
                return true;
            }
            // Return true only if the azmaterial was generated from a ".material" file.
            // Materials with subid == 0, are 99.99% guaranteed to be generated from a ".material" file.
            // Without this assurance We would need to call  AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID()
            // to figure out what's the source of this azmaterial. But, Atom can not include AzToolsFramework.
            return assetInfo.m_assetId.m_subId == 0;
        });

        m_modelBrowser.SetFilter([](const AZ::Data::AssetInfo& assetInfo)
        {
            return assetInfo.m_assetType == azrtti_typeid<AZ::RPI::ModelAsset>();
        });

        m_materialBrowser.Activate();
        m_modelBrowser.Activate();
        m_imguiSidebar.Activate();

        InitLightingPresets(true);

        AZ::Data::Asset<AZ::RPI::MaterialAsset> groundPlaneMaterialAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>(DefaultPbrMaterialPath, AZ::RPI::AssetUtils::TraceLevel::Error);
        m_groundPlaneMaterial = AZ::RPI::Material::FindOrCreate(groundPlaneMaterialAsset);
        m_groundPlaneModelAsset = AZ::RPI::AssetUtils::GetAssetByProductPath<AZ::RPI::ModelAsset>("objects/plane.azmodel", AZ::RPI::AssetUtils::TraceLevel::Assert);

        AZ::TickBus::Handler::BusConnect();
        AZ::Render::Bootstrap::DefaultWindowNotificationBus::Handler::BusConnect();
        CreateLowEndPipeline();
    }

    void MeshletsExampleComponent::Deactivate()
    {
        if (m_useLowEndPipeline)
        {
            DeactivateLowEndPipeline();
        }
        DestroyLowEndPipeline();
        AZ::Render::Bootstrap::DefaultWindowNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        m_imguiSidebar.Deactivate();

        m_materialBrowser.Deactivate();
        m_modelBrowser.Deactivate();

        RemoveController();
        
        GetMeshFeatureProcessor()->ReleaseMesh(m_meshHandle);
        GetMeshFeatureProcessor()->ReleaseMesh(m_groundPlandMeshHandle);

        if (m_meshetModel)
        {
            GetMeshFeatureProcessor()->ReleaseMesh(m_meshletMeshHandle);
            delete m_meshetModel;
            m_meshetModel = nullptr;
        }

        m_modelAsset = {};
        m_groundPlaneModelAsset = {};

        m_materialOverrideInstance = nullptr;

        ShutdownLightingPresets();
    }

    void MeshletsExampleComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        bool modelNeedsUpdate = false;

        // Switch pipeline before any imGui actions (switching pipelines switches imGui scope)
        if (m_switchPipeline)
        {
            if (m_useLowEndPipeline)
            {
                ActivateLowEndPipeline();
            }
            else
            {
                DeactivateLowEndPipeline();
            }

            m_switchPipeline = false;
        }

        if (m_imguiSidebar.Begin())
        {
            ImGuiLightingPreset();

            ImGuiAssetBrowser::WidgetSettings assetBrowserSettings;

            m_switchPipeline = ScriptableImGui::Checkbox("Use Low End Pipeline", &m_useLowEndPipeline) || m_switchPipeline;

            modelNeedsUpdate |= ScriptableImGui::Checkbox("Enable Material Override", &m_enableMaterialOverride);
           
            if (ScriptableImGui::Checkbox("Show Ground Plane", &m_showGroundPlane))
            {
                if (m_showGroundPlane)
                {
                    CreateGroundPlane();
                    UpdateGroundPlane();
                }
                else
                {
                    RemoveGroundPlane();
                }
            }

            if (ScriptableImGui::Checkbox("Show Model Materials", &m_showModelMaterials))
            {
                modelNeedsUpdate = true;
                m_materialBrowser.SetNeedsRefresh();
            }

            assetBrowserSettings.m_labels.m_root = "Materials";
            modelNeedsUpdate |= m_materialBrowser.Tick(assetBrowserSettings);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            assetBrowserSettings.m_labels.m_root = "Models";
            bool modelChanged = m_modelBrowser.Tick(assetBrowserSettings);
            modelNeedsUpdate |= modelChanged;

            if (modelChanged)
            {
                // Reset LOD override when the model changes.
                m_lodConfig.m_lodType = AZ::RPI::Cullable::LodType::Default;
            }
            /*
            AZ::Data::Instance<AZ::RPI::Model> model = GetMeshFeatureProcessor()->GetModel(m_meshHandle);
            if (model)
            {
                const char* NoLodOverrideText = "No LOD Override";
                const char* LodFormatString = "LOD %i";

                AZStd::string previewText = m_lodConfig.m_lodType == AZ::RPI::Cullable::LodType::Default ? 
                    NoLodOverrideText : 
                    AZStd::string::format(LodFormatString, m_lodConfig.m_lodOverride);

                if (ScriptableImGui::BeginCombo("", previewText.c_str()))
                {
                    if (ScriptableImGui::Selectable(NoLodOverrideText, m_lodConfig.m_lodType == AZ::RPI::Cullable::LodType::Default))
                    {
                        m_lodConfig.m_lodType = AZ::RPI::Cullable::LodType::Default;
                        GetMeshFeatureProcessor()->SetMeshLodConfiguration(m_meshHandle, m_lodConfig);
                    }

                    for (uint32_t i = 0; i < model->GetLodCount(); ++i)
                    {
                        AZStd::string name = AZStd::string::format(LodFormatString, i);
                        if (ScriptableImGui::Selectable(name.c_str(), m_lodConfig.m_lodOverride == i))
                        {
                            m_lodConfig.m_lodType = AZ::RPI::Cullable::LodType::SpecificLod;
                            m_lodConfig.m_lodOverride = static_cast<AZ::RPI::Cullable::LodOverride>(i);
                            GetMeshFeatureProcessor()->SetMeshLodConfiguration(m_meshHandle, m_lodConfig);
                        }
                    }
                    ScriptableImGui::EndCombo();
                }
            }
            */
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Camera controls
            {
                int32_t* currentControllerTypeIndex = reinterpret_cast<int32_t*>(&m_currentCameraControllerType);

                ImGui::LabelText("##CameraControllerLabel", "Camera Controller:");
                if (ScriptableImGui::Combo("##CameraController", currentControllerTypeIndex, CameraControllerNameTable, CameraControllerCount))
                {
                    ResetCameraController();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (m_materialOverrideInstance && ImGui::Button("Material Details..."))
            {
                m_imguiMaterialDetails.SetMaterial(m_materialOverrideInstance);
                m_imguiMaterialDetails.OpenDialog();
            }

            m_imguiSidebar.End();
        }

        m_imguiMaterialDetails.Tick();

        if (modelNeedsUpdate)
        {
            ModelChange();
        }
    }

    void MeshletsExampleComponent::ModelChange()
    {
        if (!m_modelBrowser.GetSelectedAssetId().IsValid())
        {
            m_modelAsset = {};
            GetMeshFeatureProcessor()->ReleaseMesh(m_meshHandle);
            GetMeshFeatureProcessor()->ReleaseMesh(m_meshletMeshHandle);
            return;
        }

        // If a material hasn't been selected, just choose the first one
        // If for some reason no materials are available log an error
        AZ::Data::AssetId selectedMaterialAssetId = m_materialBrowser.GetSelectedAssetId();
        if (!selectedMaterialAssetId.IsValid())
        {
            selectedMaterialAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(DefaultPbrMaterialPath, AZ::RPI::AssetUtils::TraceLevel::Error);

            if (!selectedMaterialAssetId.IsValid())
            {
                AZ_Error("MeshExampleComponent", false, "Failed to select model, no material available to render with.");
                return;
            }
        }

        if (m_enableMaterialOverride && selectedMaterialAssetId.IsValid())
        {
            AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset;
            materialAsset.Create(selectedMaterialAssetId);
            m_materialOverrideInstance = AZ::RPI::Material::FindOrCreate(materialAsset);
        }
        else
        {
            m_materialOverrideInstance = nullptr;
        }


        if (m_modelAsset.GetId() != m_modelBrowser.GetSelectedAssetId())
        {
            ScriptRunnerRequestBus::Broadcast(&ScriptRunnerRequests::PauseScript);

            m_modelAsset.Create(m_modelBrowser.GetSelectedAssetId());

            GetMeshFeatureProcessor()->ReleaseMesh(m_meshHandle);

            if (m_meshetModel)
            {   // delete the meshlet model so it will be recreated on the next tick
                GetMeshFeatureProcessor()->ReleaseMesh(m_meshletMeshHandle);
                delete m_meshetModel;
                m_meshetModel = nullptr;
            }

            m_meshHandle = GetMeshFeatureProcessor()->AcquireMesh(AZ::Render::MeshHandleDescriptor{ m_modelAsset }, m_materialOverrideInstance);

            GetMeshFeatureProcessor()->SetTransform(m_meshHandle, AZ::Transform::CreateIdentity());
            GetMeshFeatureProcessor()->ConnectModelChangeEventHandler(m_meshHandle, m_changedHandler);
            GetMeshFeatureProcessor()->SetMeshLodConfiguration(m_meshHandle, m_lodConfig);
        }
        else
        {
            GetMeshFeatureProcessor()->SetMaterialAssignmentMap(m_meshHandle, m_materialOverrideInstance);
        }
    }
    
    void MeshletsExampleComponent::CreateGroundPlane()
    {
        m_groundPlandMeshHandle = GetMeshFeatureProcessor()->AcquireMesh(AZ::Render::MeshHandleDescriptor{ m_groundPlaneModelAsset }, m_groundPlaneMaterial);
    }

    void MeshletsExampleComponent::UpdateGroundPlane()
    {
        if (m_groundPlandMeshHandle.IsValid())
        {
            AZ::Transform groundPlaneTransform = AZ::Transform::CreateIdentity();

            if (m_modelAsset)
            {
                AZ::Vector3 modelCenter;
                float modelRadius;
                m_modelAsset->GetAabb().GetAsSphere(modelCenter, modelRadius);

                static const float GroundPlaneRelativeScale = 4.0f;
                static const float GroundPlaneOffset = 0.01f;

                groundPlaneTransform.SetUniformScale(GroundPlaneRelativeScale * modelRadius);
                groundPlaneTransform.SetTranslation(AZ::Vector3(0.0f, 0.0f, m_modelAsset->GetAabb().GetMin().GetZ() - GroundPlaneOffset));
            }

            GetMeshFeatureProcessor()->SetTransform(m_groundPlandMeshHandle, groundPlaneTransform);
        }
    }

    void MeshletsExampleComponent::RemoveGroundPlane()
    {
        GetMeshFeatureProcessor()->ReleaseMesh(m_groundPlandMeshHandle);
    }

    void MeshletsExampleComponent::OnEntityDestruction(const AZ::EntityId& entityId)
    {
        OnLightingPresetEntityShutdown(entityId);
        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);
    }

    void MeshletsExampleComponent::UseArcBallCameraController()
    {
        AZ::Debug::CameraControllerRequestBus::Event(GetCameraEntityId(), &AZ::Debug::CameraControllerRequestBus::Events::Enable,
            azrtti_typeid<AZ::Debug::ArcBallControllerComponent>());
    }

    void MeshletsExampleComponent::UseNoClipCameraController()
    {
        AZ::Debug::CameraControllerRequestBus::Event(GetCameraEntityId(), &AZ::Debug::CameraControllerRequestBus::Events::Enable,
            azrtti_typeid<AZ::Debug::NoClipControllerComponent>());
    }

    void MeshletsExampleComponent::RemoveController()
    {
        AZ::Debug::CameraControllerRequestBus::Event(GetCameraEntityId(), &AZ::Debug::CameraControllerRequestBus::Events::Disable);
    }

    void MeshletsExampleComponent::SetArcBallControllerParams()
    {
        if (!m_modelBrowser.GetSelectedAssetId().IsValid() || !m_modelAsset.IsReady())
        {
            return;
        }


        if (!m_meshetModel)
        {
            m_meshetModel = new AZ::Meshlets::MeshletModel(m_modelAsset);
            if (m_meshetModel->GetMeshletModel())
            {
                static constexpr const char meshletDebugMaterialPath[] = "objects/adi/debugshadermaterial_01.azmaterial";

                AZ::Data::Asset<AZ::RPI::MaterialAsset> meshletDebugMaterialAsset =
                    AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>(meshletDebugMaterialPath, AZ::RPI::AssetUtils::TraceLevel::Error);

                m_meshletDebugMaterial = AZ::RPI::Material::FindOrCreate(meshletDebugMaterialAsset);

                m_meshletModelAsset = m_meshetModel->GetMeshletModel()->GetModelAsset();
                m_meshletMeshHandle = GetMeshFeatureProcessor()->AcquireMesh(AZ::Render::MeshHandleDescriptor{ m_meshletModelAsset }, m_meshletDebugMaterial);// m_materialOverrideInstance);

                AZ::Transform translation = AZ::Transform::CreateTranslation(AZ::Vector3(0, 2.0, 0));
                GetMeshFeatureProcessor()->SetTransform(m_meshletMeshHandle, translation);
            }
        }

        // Adjust the arc-ball controller so that it has bounds that make sense for the current model
        
        AZ::Vector3 center;
        float radius;
        m_modelAsset->GetAabb().GetAsSphere(center, radius);

        const float startingDistance = radius * ArcballRadiusDefaultModifier;
        const float minDistance = radius * ArcballRadiusMinModifier;
        const float maxDistance = radius * ArcballRadiusMaxModifier;

        AZ::Debug::ArcBallControllerRequestBus::Event(GetCameraEntityId(), &AZ::Debug::ArcBallControllerRequestBus::Events::SetCenter, center);
        AZ::Debug::ArcBallControllerRequestBus::Event(GetCameraEntityId(), &AZ::Debug::ArcBallControllerRequestBus::Events::SetDistance, startingDistance);
        AZ::Debug::ArcBallControllerRequestBus::Event(GetCameraEntityId(), &AZ::Debug::ArcBallControllerRequestBus::Events::SetMinDistance, minDistance);
        AZ::Debug::ArcBallControllerRequestBus::Event(GetCameraEntityId(), &AZ::Debug::ArcBallControllerRequestBus::Events::SetMaxDistance, maxDistance);
    }
    void MeshletsExampleComponent::ResetCameraController()
    {
        RemoveController();
        if (m_currentCameraControllerType == CameraControllerType::ArcBall)
        {
            UseArcBallCameraController();
            SetArcBallControllerParams();
        }
        else if (m_currentCameraControllerType == CameraControllerType::NoClip)
        {
            UseNoClipCameraController();
        }
    }
} // namespace AtomSampleViewer
