/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#undef RC_INVOKED

#include <Atom/Component/DebugCamera/CameraComponent.h>
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcessing/PostProcessingConstants.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConstants.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/DollyCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/IdleBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/MoveCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/OrbitCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/PanCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateModelBehavior.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <Document/MaterialDocumentRequestBus.h>
#include <Viewport/MaterialViewportSettingsRequestBus.h>
#include <Viewport/MaterialViewportWidget.h>

namespace MaterialEditor
{
    static constexpr float DepthNear = 0.01f;

    MaterialViewportWidget::MaterialViewportWidget(const AZ::Crc32& toolId, QWidget* parent)
        : AtomToolsFramework::RenderViewportWidget(parent)
        , m_toolId(toolId)
    {
        // The viewport context created by AtomToolsFramework::RenderViewportWidget has no name.
        // Systems like frame capturing and post FX expect there to be a context with DefaultViewportContextName
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        const AZ::Name defaultContextName = viewportContextManager->GetDefaultViewportContextName();
        viewportContextManager->RenameViewportContext(GetViewportContext(), defaultContextName);

        // Create a custom entity context for the entities in this viewport
        m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
        m_entityContext->InitContext();

        // Create and register a scene with all available feature processors
        AZ::RPI::SceneDescriptor sceneDesc;
        sceneDesc.m_nameId = AZ::Name(AZStd::string::format("MaterialViewportWidgetScene_%i", GetViewportContext()->GetId()));
        m_scene = AZ::RPI::Scene::CreateScene(sceneDesc);
        m_scene->EnableAllFeatureProcessors();

        // Bind m_frameworkScene to the entity context's AzFramework::Scene
        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "MaterialViewportWidget was unable to get the scene system during construction.");

        AZ::Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> createSceneOutcome =
            sceneSystem->CreateScene(AZStd::string::format("MaterialViewportWidgetScene_%i", GetViewportContext()->GetId()));
        AZ_Assert(createSceneOutcome, createSceneOutcome.GetError().c_str());

        m_frameworkScene = createSceneOutcome.TakeValue();
        m_frameworkScene->SetSubsystem(m_scene);
        m_frameworkScene->SetSubsystem(m_entityContext.get());

        // Load the render pipeline asset
        AZStd::optional<AZ::RPI::RenderPipelineDescriptor> mainPipelineDesc = AZ::RPI::GetRenderPipelineDescriptorFromAsset(
            m_mainPipelineAssetPath, AZStd::string::format("_%i", GetViewportContext()->GetId()));
        AZ_Assert(mainPipelineDesc.has_value(), "Invalid render pipeline descriptor from asset %s", m_mainPipelineAssetPath.c_str());

        // TODO SetApplicationMultisampleState should only be called once per application and will need to consider multiple viewports and
        // pipelines. The default pipeline determines the initial MSAA state for the application.
        AZ::RPI::RPISystemInterface::Get()->SetApplicationMultisampleState(mainPipelineDesc.value().m_renderSettings.m_multisampleState);
        mainPipelineDesc.value().m_renderSettings.m_multisampleState = AZ::RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();

        // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene
        m_renderPipeline = AZ::RPI::RenderPipeline::CreateRenderPipelineForWindow(
            mainPipelineDesc.value(), *GetViewportContext()->GetWindowContext().get());
        m_scene->AddRenderPipeline(m_renderPipeline);

        // Create the BRDF texture generation pipeline
        AZ::RPI::RenderPipelineDescriptor brdfPipelineDesc;
        brdfPipelineDesc.m_mainViewTagName = "MainCamera";
        brdfPipelineDesc.m_name = AZStd::string::format("BRDFTexturePipeline_%i", GetViewportContext()->GetId());
        brdfPipelineDesc.m_rootPassTemplate = "BRDFTexturePipeline";
        brdfPipelineDesc.m_renderSettings.m_multisampleState = AZ::RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();
        brdfPipelineDesc.m_executeOnce = true;

        AZ::RPI::RenderPipelinePtr brdfTexturePipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(brdfPipelineDesc);
        m_scene->AddRenderPipeline(brdfTexturePipeline);
        m_scene->Activate();

        AZ::RPI::RPISystemInterface::Get()->RegisterScene(m_scene);

        // Configure camera
        m_cameraEntity =
            CreateEntity("Cameraentity", { azrtti_typeid<AzFramework::TransformComponent>(), azrtti_typeid<AZ::Debug::CameraComponent>() });

        AZ::Debug::CameraComponentConfig cameraConfig(GetViewportContext()->GetWindowContext());
        cameraConfig.m_fovY = AZ::Constants::HalfPi;
        cameraConfig.m_depthNear = DepthNear;
        m_cameraEntity->Deactivate();
        m_cameraEntity->FindComponent(azrtti_typeid<AZ::Debug::CameraComponent>())->SetConfiguration(cameraConfig);
        m_cameraEntity->Activate();

        // Connect camera to pipeline's default view after camera entity activated
        m_renderPipeline->SetDefaultViewFromEntity(m_cameraEntity->GetId());

        // Configure tone mapper
        m_postProcessEntity = CreateEntity(
            "PostProcessEntity",
            { AZ::Render::PostFxLayerComponentTypeId, AZ::Render::ExposureControlComponentTypeId,
              azrtti_typeid<AzFramework::TransformComponent>() });

        // Init directional light processor
        m_directionalLightFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>();

        // Init display mapper processor
        m_displayMapperFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DisplayMapperFeatureProcessorInterface>();

        // Init Skybox
        m_skyboxFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
        m_skyboxFeatureProcessor->Enable(true);
        m_skyboxFeatureProcessor->SetSkyboxMode(AZ::Render::SkyBoxMode::Cubemap);

        // Create IBL
        m_iblEntity =
            CreateEntity("IblEntity", { AZ::Render::ImageBasedLightComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>() });

        // Create model
        m_modelEntity = CreateEntity(
            "ViewportModel",
            { AZ::Render::MeshComponentTypeId, AZ::Render::MaterialComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>() });

        // Create shadow catcher
        m_shadowCatcherEntity = CreateEntity(
            "ViewportShadowCatcher",
            { AZ::Render::MeshComponentTypeId, AZ::Render::MaterialComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>(),
              azrtti_typeid<AzFramework::NonUniformScaleComponent>() });

        AZ::NonUniformScaleRequestBus::Event(
            m_shadowCatcherEntity->GetId(), &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3{ 100, 100, 1.0 });

        AZ::Data::AssetId shadowCatcherModelAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(
            "materialeditor/viewportmodels/plane_1x1.azmodel", AZ::RPI::AssetUtils::TraceLevel::Error);
        AZ::Render::MeshComponentRequestBus::Event(
            m_shadowCatcherEntity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::SetModelAssetId, shadowCatcherModelAssetId);

        auto shadowCatcherMaterialAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>(
            "materials/special/shadowcatcher.azmaterial", AZ::RPI::AssetUtils::TraceLevel::Error);
        if (shadowCatcherMaterialAsset)
        {
            m_shadowCatcherOpacityPropertyIndex =
                shadowCatcherMaterialAsset->GetMaterialTypeAsset()->GetMaterialPropertiesLayout()->FindPropertyIndex(
                    AZ::Name{ "settings.opacity" });
            AZ_Error("MaterialViewportWidget", m_shadowCatcherOpacityPropertyIndex.IsValid(), "Could not find opacity property");

            m_shadowCatcherMaterial = AZ::RPI::Material::Create(shadowCatcherMaterialAsset);
            AZ_Error("MaterialViewportWidget", m_shadowCatcherMaterial != nullptr, "Could not create shadow catcher material.");

            AZ::Render::MaterialAssignmentMap shadowCatcherMaterials;
            auto& shadowCatcherMaterialAssignment = shadowCatcherMaterials[AZ::Render::DefaultMaterialAssignmentId];
            shadowCatcherMaterialAssignment.m_materialInstance = m_shadowCatcherMaterial;
            shadowCatcherMaterialAssignment.m_materialInstancePreCreated = true;

            AZ::Render::MaterialComponentRequestBus::Event(
                m_shadowCatcherEntity->GetId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialOverrides,
                shadowCatcherMaterials);
        }

        // Create grid
        m_gridEntity = CreateEntity("ViewportGrid", { AZ::Render::GridComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>() });

        AZ::Render::GridComponentConfig gridConfig;
        gridConfig.m_gridSize = 4.0f;
        gridConfig.m_axisColor = AZ::Color(0.1f, 0.1f, 0.1f, 1.0f);
        gridConfig.m_primaryColor = AZ::Color(0.1f, 0.1f, 0.1f, 1.0f);
        gridConfig.m_secondaryColor = AZ::Color(0.1f, 0.1f, 0.1f, 1.0f);
        m_gridEntity->Deactivate();
        m_gridEntity->FindComponent(AZ::Render::GridComponentTypeId)->SetConfiguration(gridConfig);
        m_gridEntity->Activate();

        SetupInputController();

        OnDocumentOpened(AZ::Uuid::CreateNull());

        // Apply user settinngs restored since last run
        OnViewportSettingsChanged();

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
        MaterialViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_cameraEntity->GetId());
        AZ::TickBus::Handler::BusConnect();
    }

    MaterialViewportWidget::~MaterialViewportWidget()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        MaterialViewportSettingsNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();

        DestroyEntity(m_iblEntity);
        DestroyEntity(m_modelEntity);
        DestroyEntity(m_shadowCatcherEntity);
        DestroyEntity(m_gridEntity);
        DestroyEntity(m_cameraEntity);
        DestroyEntity(m_postProcessEntity);

        for (DirectionalLightHandle& handle : m_lightHandles)
        {
            m_directionalLightFeatureProcessor->ReleaseLight(handle);
        }
        m_lightHandles.clear();

        m_scene->Deactivate();
        m_scene->RemoveRenderPipeline(m_renderPipeline->GetId());
        AZ::RPI::RPISystemInterface::Get()->UnregisterScene(m_scene);
        m_frameworkScene->UnsetSubsystem(m_scene);
        m_frameworkScene->UnsetSubsystem(m_entityContext.get());

        if (auto sceneSystem = AzFramework::SceneSystemInterface::Get())
        {
            sceneSystem->RemoveScene(m_frameworkScene->GetName());
        }
    }

    AZ::Entity* MaterialViewportWidget::CreateEntity(const AZStd::string& name, const AZStd::vector<AZ::Uuid>& componentTypeIds)
    {
        AZ::Entity* entity = {};
        AzFramework::EntityContextRequestBus::EventResult(
            entity, m_entityContext->GetContextId(), &AzFramework::EntityContextRequestBus::Events::CreateEntity, name.c_str());
        AZ_Assert(entity != nullptr, "Failed to create post process entity: %s.", name.c_str());

        if (entity)
        {
            for (const auto& componentTypeId : componentTypeIds)
            {
                entity->CreateComponent(componentTypeId);
            }
            entity->Init();
            entity->Activate();
        }

        return entity;
    }

    void MaterialViewportWidget::DestroyEntity(AZ::Entity*& entity)
    {
        AzFramework::EntityContextRequestBus::Event(
            m_entityContext->GetContextId(), &AzFramework::EntityContextRequestBus::Events::DestroyEntity, entity);
        entity = nullptr;
    }

    void MaterialViewportWidget::SetupInputController()
    {
        using namespace AtomToolsFramework;

        // Create viewport input controller and regioster its behaviors
        m_viewportController.reset(
            aznew ViewportInputBehaviorController(m_cameraEntity->GetId(), m_modelEntity->GetId(), m_iblEntity->GetId()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::None, AZStd::make_shared<IdleBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Lmb, AZStd::make_shared<PanCameraBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Mmb, AZStd::make_shared<MoveCameraBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Rmb, AZStd::make_shared<OrbitCameraBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Alt ^ ViewportInputBehaviorController::Lmb,
            AZStd::make_shared<OrbitCameraBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Alt ^ ViewportInputBehaviorController::Mmb,
            AZStd::make_shared<MoveCameraBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Alt ^ ViewportInputBehaviorController::Rmb,
            AZStd::make_shared<DollyCameraBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Lmb ^ ViewportInputBehaviorController::Rmb,
            AZStd::make_shared<DollyCameraBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Ctrl ^ ViewportInputBehaviorController::Lmb,
            AZStd::make_shared<RotateModelBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Shift ^ ViewportInputBehaviorController::Lmb,
            AZStd::make_shared<RotateEnvironmentBehavior>(m_viewportController.get()));

        GetControllerList()->Add(m_viewportController);
    }

    void MaterialViewportWidget::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        AZ::Data::Instance<AZ::RPI::Material> materialInstance;
        MaterialDocumentRequestBus::EventResult(materialInstance, documentId, &MaterialDocumentRequestBus::Events::GetInstance);

        AZ::Render::MaterialAssignmentMap materials;
        auto& materialAssignment = materials[AZ::Render::DefaultMaterialAssignmentId];
        materialAssignment.m_materialInstance = materialInstance;
        materialAssignment.m_materialInstancePreCreated = true;

        AZ::Render::MaterialComponentRequestBus::Event(
            m_modelEntity->GetId(), &AZ::Render::MaterialComponentRequestBus::Events::SetMaterialOverrides, materials);
    }

    void MaterialViewportWidget::OnViewportSettingsChanged()
    {
        MaterialViewportSettingsRequestBus::Event(
            m_toolId,
            [this](MaterialViewportSettingsRequestBus::Events* viewportRequests)
            {
                UpdateLighting(viewportRequests);
                UpdateModel(viewportRequests);
                UpdateGrid(viewportRequests);

                AZ::Render::MeshComponentRequestBus::Event(
                    m_shadowCatcherEntity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::SetVisibility,
                    viewportRequests->GetShadowCatcherEnabled());

                m_viewportController->SetFieldOfView(viewportRequests->GetFieldOfView());

                AZ::Render::DisplayMapperConfigurationDescriptor desc;
                desc.m_operationType = viewportRequests->GetDisplayMapperOperationType();
                m_displayMapperFeatureProcessor->RegisterDisplayMapperConfiguration(desc);
            });
    }

    void MaterialViewportWidget::UpdateLighting(MaterialViewportRequests* viewportRequests)
    {
        auto iblFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::ImageBasedLightFeatureProcessorInterface>();
        auto postProcessFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::PostProcessFeatureProcessorInterface>();
        auto postProcessSettingInterface = postProcessFeatureProcessor->GetOrCreateSettingsInterface(m_postProcessEntity->GetId());
        auto exposureControlSettingInterface = postProcessSettingInterface->GetOrCreateExposureControlSettingsInterface();

        Camera::Configuration cameraConfig;
        Camera::CameraRequestBus::EventResult(
            cameraConfig, m_cameraEntity->GetId(), &Camera::CameraRequestBus::Events::GetCameraConfiguration);

        const bool enableAlternateSkybox = viewportRequests->GetAlternateSkyboxEnabled();
        viewportRequests->GetLightingPreset().ApplyLightingPreset(
            iblFeatureProcessor, m_skyboxFeatureProcessor, exposureControlSettingInterface, m_directionalLightFeatureProcessor,
            cameraConfig, m_lightHandles, m_shadowCatcherMaterial, m_shadowCatcherOpacityPropertyIndex, enableAlternateSkybox);
    }

    void MaterialViewportWidget::UpdateModel(MaterialViewportRequests* viewportRequests)
    {
        const auto& modelPreset = viewportRequests->GetModelPreset();
        if (modelPreset.m_modelAsset.GetId().IsValid() && modelPreset.m_modelAsset != m_modelAsset)
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();

            m_modelAsset = modelPreset.m_modelAsset;
            AZ::Data::AssetBus::Handler::BusConnect(m_modelAsset.GetId());

            AZ::Render::MeshComponentRequestBus::Event(
                m_modelEntity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::SetModelAsset, m_modelAsset);
        }
    }

    void MaterialViewportWidget::UpdateGrid(MaterialViewportRequests* viewportRequests)
    {
        if (m_gridEntity)
        {
            const bool enableGrid = viewportRequests->GetGridEnabled();
            if (enableGrid && m_gridEntity->GetState() == AZ::Entity::State::Init)
            {
                m_gridEntity->Activate();
                return;
            }

            if (!enableGrid && m_gridEntity->GetState() == AZ::Entity::State::Active)
            {
                m_gridEntity->Deactivate();
                return;
            }
        }
    }

    void MaterialViewportWidget::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_modelAsset == asset)
        {
            m_modelAsset = asset;
            m_viewportController->SetTargetBounds(m_modelAsset->GetAabb());
            m_viewportController->Reset();
            AZ::Data::AssetBus::Handler::BusDisconnect(m_modelAsset.GetId());
        }
    }

    void MaterialViewportWidget::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AtomToolsFramework::RenderViewportWidget::OnTick(deltaTime, time);

        m_renderPipeline->AddToRenderTickOnce();

        if (m_shadowCatcherMaterial)
        {
            // Compile the m_shadowCatcherMaterial in OnTick because changes can only be compiled once per frame.
            // This is ignored when a compile isn't needed.
            m_shadowCatcherMaterial->Compile();
        }
    }

    void MaterialViewportWidget::OnTransformChanged(const AZ::Transform&, const AZ::Transform&)
    {
        const AZ::EntityId* currentBusId = AZ::TransformNotificationBus::GetCurrentBusId();
        if (m_cameraEntity && currentBusId && *currentBusId == m_cameraEntity->GetId() && m_directionalLightFeatureProcessor)
        {
            auto transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, m_cameraEntity->GetId(), &AZ::TransformBus::Events::GetWorldTM);
            for (const DirectionalLightHandle& id : m_lightHandles)
            {
                m_directionalLightFeatureProcessor->SetCameraTransform(id, transform);
            }
        }
    }
} // namespace MaterialEditor
