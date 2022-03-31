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
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcessing/PostProcessingConstants.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportWidget.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/DollyCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/IdleBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/MoveCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/OrbitCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/PanCameraBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateObjectBehavior.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Viewport/ViewportControllerList.h>

namespace AtomToolsFramework
{
    EntityPreviewViewportWidget::EntityPreviewViewportWidget(
        const AZ::Crc32& toolId, const AZStd::string& sceneName, const AZStd::string pipelineAssetPath, QWidget* parent)
        : RenderViewportWidget(parent)
        , m_toolId(toolId)
        , m_sceneName(sceneName)
        , m_pipelineAssetPath(pipelineAssetPath)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        AZ::TickBus::Handler::BusConnect();
        EntityPreviewViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
    }

    EntityPreviewViewportWidget::~EntityPreviewViewportWidget()
    {
        EntityPreviewViewportSettingsNotificationBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        DestroyEntities();
        DestroyScene();
    }

    void EntityPreviewViewportWidget::Init()
    {
        // Create a custom entity context for the entities in this viewport
        m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
        m_entityContext->InitContext();

        CreateScene();
        CreateEntities();
        CreateInputController();
        OnViewportSettingsChanged();
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetCameraEntityId());
    }

    AZ::Aabb EntityPreviewViewportWidget::GetObjectBoundsLocal() const
    {
        return AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
    }

    AZ::Aabb EntityPreviewViewportWidget::GetObjectBoundsWorld() const
    {
        return AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f);
    }

    AZ::EntityId EntityPreviewViewportWidget::GetObjectEntityId() const
    {
        return AZ::EntityId();
    }

    AZ::EntityId EntityPreviewViewportWidget::GetCameraEntityId() const
    {
        return m_cameraEntity ? m_cameraEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId EntityPreviewViewportWidget::GetEnvironmentEntityId() const
    {
        return AZ::EntityId();
    }

    AZ::EntityId EntityPreviewViewportWidget::GetPostFxEntityId() const
    {
        return AZ::EntityId();
    }

    AZ::Entity* EntityPreviewViewportWidget::CreateEntity(const AZStd::string& name, const AZStd::vector<AZ::Uuid>& componentTypeIds)
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
            m_entities.push_back(entity);
        }

        return entity;
    }

    void EntityPreviewViewportWidget::DestroyEntity(AZ::Entity*& entity)
    {
        if (entity)
        {
            m_entities.erase(AZStd::remove(m_entities.begin(), m_entities.end(), entity), m_entities.end());
            AzFramework::EntityContextRequestBus::Event(
                m_entityContext->GetContextId(), &AzFramework::EntityContextRequestBus::Events::DestroyEntity, entity);
            entity = nullptr;
        }
    }

    void EntityPreviewViewportWidget::CreateScene()
    {
        // The viewport context created by RenderViewportWidget has no name.
        // Systems like frame capturing and post FX expect there to be a context with DefaultViewportContextName
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        const AZ::Name defaultContextName = viewportContextManager->GetDefaultViewportContextName();
        viewportContextManager->RenameViewportContext(GetViewportContext(), defaultContextName);

        // Create and register a scene with all available feature processors
        AZ::RPI::SceneDescriptor sceneDesc;
        sceneDesc.m_nameId = AZ::Name(AZStd::string::format("%s_%i", m_sceneName.c_str(), GetViewportContext()->GetId()));
        m_scene = AZ::RPI::Scene::CreateScene(sceneDesc);
        m_scene->EnableAllFeatureProcessors();

        // Bind m_frameworkScene to the entity context's AzFramework::Scene
        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "EntityPreviewViewportWidget was unable to get the scene system during construction.");

        AZ::Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> createSceneOutcome =
            sceneSystem->CreateScene(AZStd::string::format("%s_%i", m_sceneName.c_str(), GetViewportContext()->GetId()));
        AZ_Assert(createSceneOutcome, createSceneOutcome.GetError().c_str());

        m_frameworkScene = createSceneOutcome.TakeValue();
        m_frameworkScene->SetSubsystem(m_scene);
        m_frameworkScene->SetSubsystem(m_entityContext.get());

        // Load the render pipeline asset
        AZStd::optional<AZ::RPI::RenderPipelineDescriptor> mainPipelineDesc =
            AZ::RPI::GetRenderPipelineDescriptorFromAsset(m_pipelineAssetPath, AZStd::string::format("_%i", GetViewportContext()->GetId()));
        AZ_Assert(mainPipelineDesc.has_value(), "Invalid render pipeline descriptor from asset %s", m_pipelineAssetPath.c_str());

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
        brdfPipelineDesc.m_name = AZStd::string::format("%s_%i_BRDFTexturePipeline", m_sceneName.c_str(), GetViewportContext()->GetId());
        brdfPipelineDesc.m_rootPassTemplate = "BRDFTexturePipeline";
        brdfPipelineDesc.m_renderSettings.m_multisampleState = AZ::RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();
        brdfPipelineDesc.m_executeOnce = true;

        AZ::RPI::RenderPipelinePtr brdfTexturePipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(brdfPipelineDesc);
        m_scene->AddRenderPipeline(brdfTexturePipeline);
        m_scene->Activate();

        AZ::RPI::RPISystemInterface::Get()->RegisterScene(m_scene);
    }

    void EntityPreviewViewportWidget::DestroyScene()
    {
        auto directionalLightFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>();
        if (directionalLightFeatureProcessor)
        {
            for (DirectionalLightHandle& handle : m_lightHandles)
            {
                directionalLightFeatureProcessor->ReleaseLight(handle);
            }
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

    void EntityPreviewViewportWidget::CreateEntities()
    {
        // Configure camera
        m_cameraEntity =
            CreateEntity("Cameraentity", { azrtti_typeid<AzFramework::TransformComponent>(), azrtti_typeid<AZ::Debug::CameraComponent>() });

        AZ::Debug::CameraComponentConfig cameraConfig(GetViewportContext()->GetWindowContext());
        cameraConfig.m_fovY = AZ::Constants::HalfPi;
        cameraConfig.m_depthNear = 0.01f;
        m_cameraEntity->Deactivate();
        m_cameraEntity->FindComponent(azrtti_typeid<AZ::Debug::CameraComponent>())->SetConfiguration(cameraConfig);
        m_cameraEntity->Activate();

        // Connect camera to pipeline's default view after camera entity activated
        m_renderPipeline->SetDefaultViewFromEntity(GetCameraEntityId());
    }

    void EntityPreviewViewportWidget::DestroyEntities()
    {
        while (!m_entities.empty())
        {
            DestroyEntity(m_entities.back());
        }
    }

    void EntityPreviewViewportWidget::CreateInputController()
    {
        // Create viewport input controller and regioster its behaviors
        m_viewportController.reset(
            aznew ViewportInputBehaviorController(this, GetCameraEntityId(), GetObjectEntityId(), GetEnvironmentEntityId()));
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
            AZStd::make_shared<RotateObjectBehavior>(m_viewportController.get()));
        m_viewportController->AddBehavior(
            ViewportInputBehaviorController::Shift ^ ViewportInputBehaviorController::Lmb,
            AZStd::make_shared<RotateEnvironmentBehavior>(m_viewportController.get()));
        m_viewportController->SetObjectBounds(AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 0.5f));
        m_viewportController->Reset();

        GetControllerList()->Add(m_viewportController);
    }

    void EntityPreviewViewportWidget::OnViewportSettingsChanged()
    {
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequestBus::Events* viewportRequests)
            {
                m_viewportController->SetFieldOfView(viewportRequests->GetFieldOfView());

                // Update lighting preset, skybox, and shadows
                auto iblFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::ImageBasedLightFeatureProcessorInterface>();
                auto postProcessFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::PostProcessFeatureProcessorInterface>();
                auto postProcessSettingInterface = postProcessFeatureProcessor->GetOrCreateSettingsInterface(GetPostFxEntityId());
                auto exposureControlSettingInterface = postProcessSettingInterface->GetOrCreateExposureControlSettingsInterface();
                auto directionalLightFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>();
                auto skyboxFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();

                Camera::Configuration cameraConfig;
                Camera::CameraRequestBus::EventResult(
                    cameraConfig, GetCameraEntityId(), &Camera::CameraRequestBus::Events::GetCameraConfiguration);

                const bool enableAlternateSkybox = viewportRequests->GetAlternateSkyboxEnabled();
                viewportRequests->GetLightingPreset().ApplyLightingPreset(
                    iblFeatureProcessor, skyboxFeatureProcessor, exposureControlSettingInterface, directionalLightFeatureProcessor,
                    cameraConfig, m_lightHandles, enableAlternateSkybox);
            });
    }

    void EntityPreviewViewportWidget::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        RenderViewportWidget::OnTick(deltaTime, time);

        if (m_objectBoundsOld != GetObjectBoundsLocal())
        {
            m_objectBoundsOld = GetObjectBoundsLocal();
            m_viewportController->SetObjectBounds(GetObjectBoundsWorld());
            m_viewportController->Reset();
        }


        m_renderPipeline->AddToRenderTickOnce();
    }

    void EntityPreviewViewportWidget::OnTransformChanged(const AZ::Transform&, const AZ::Transform&)
    {
        auto directionalLightFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>();
        if (directionalLightFeatureProcessor && m_cameraEntity)
        {
            const auto currentBusId = *AZ::TransformNotificationBus::GetCurrentBusId();
            if (currentBusId == GetCameraEntityId())
            {
                auto transform = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(transform, GetCameraEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                for (const DirectionalLightHandle& id : m_lightHandles)
                {
                    directionalLightFeatureProcessor->SetCameraTransform(id, transform);
                }
            }
        }
    }
} // namespace AtomToolsFramework
