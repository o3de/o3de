/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#undef RC_INVOKED

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <Atom/Component/DebugCamera/CameraComponent.h>
#include <Atom/Feature/PostProcessing/PostProcessingConstants.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>

#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <OpenParticleViewportRequestBus.h>
#include <OpenParticleViewportSettings.h>

#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConstants.h>

#include <Editor/EditorParticleRequestBus.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzFramework/Font/FontInterface.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <OpenParticleSystem/ParticleComponent.h>
#include <OpenParticleSystem/ParticleRequestBus.h>
#include <OpenParticleViewportRenderer.h>
#include <OpenParticleViewportWidgetRequestsBus.h>
#include <Editor/EditorParticleComponent.h>

namespace OpenParticleSystemEditor
{
    static constexpr float DepthNear = 0.01f;

    OpenParticleViewportRenderer::OpenParticleViewportRenderer(AZStd::shared_ptr<AZ::RPI::WindowContext> windowContext)
        : m_windowContext(windowContext)
        , m_viewportController(AZStd::make_shared<OpenParticleEditorViewportInputController>())
    {
        using namespace AZ;
        using namespace Data;

        m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
        m_entityContext->InitContext();

        CreateRenderScene();
        CreateAssetRenderPipeline();
        CreateBRDFTexturePipeline();

        // Currently the scene has to be activated after render pipeline was added so some feature processors (i.e. imgui) can be initialized properly
        // with pipeline's pass information.
        m_scene->Activate();
        AZ::RPI::RPISystemInterface::Get()->RegisterScene(m_scene);

        AzFramework::EntityContextId entityContextId = m_entityContext.get()->GetContextId();
        CreateCamera(entityContextId);
        CreateParticleEntity(entityContextId);
        CreatePostProcessEntity(entityContextId);

        // Init directional light processor
        m_directionalLightFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>();

        InitSkybox();

        CreateGrid(entityContextId);

        // Attempt to apply the default lighting preset
        AZStd::string lightingPreset;
        OpenParticleViewportRequestBus::BroadcastResult(lightingPreset, &OpenParticleViewportRequestBus::Events::GetLightingPresetSelection);
        OnLightingPresetSelected(lightingPreset);

        m_viewportController->Init(m_cameraEntity->GetId());

        // Apply user settinngs restored since last run
        AZStd::intrusive_ptr<OpenParticleViewportSettings> viewportSettings =
            AZ::UserSettings::CreateFind<OpenParticleViewportSettings>(AZ::Crc32("OpenParticleViewportSettings"), AZ::UserSettings::CT_GLOBAL);

        OnGridEnabledChanged(viewportSettings->m_enableGrid);
        OnAlternateSkyboxEnabledChanged(viewportSettings->m_enableAlternateSkybox);

        ConnectEBus();
    }

    OpenParticleViewportRenderer::~OpenParticleViewportRenderer()
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        ParticleDocumentNotifyBus::Handler::BusDisconnect();
        OpenParticleViewportNotificationBus::Handler::BusDisconnect();
        OpenParticleSystem::ParticleEditorRequestBus::Handler::BusDisconnect();

        AzFramework::EntityContextId entityContextId = m_entityContext.get()->GetContextId();
        if (!entityContextId.IsNull())
        {
            AzFramework::EntityContextRequestBus::Event(entityContextId, &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_gridEntity);
            m_gridEntity = nullptr;

            AzFramework::EntityContextRequestBus::Event(entityContextId, &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_cameraEntity);
            m_cameraEntity = nullptr;

            AzFramework::EntityContextRequestBus::Event(entityContextId, &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_postProcessEntity);
            m_postProcessEntity = nullptr;

            AzFramework::EntityContextRequestBus::Event(entityContextId, &AzFramework::EntityContextRequestBus::Events::DestroyEntity, m_particleEntity);
            m_particleEntity = nullptr;
        }

        for (DirectionalLightHandle& handle : m_lightHandles)
        {
            m_directionalLightFeatureProcessor->ReleaseLight(handle);
        }
        m_lightHandles.clear();

        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "OpenParticleViewportRenderer was unable to get the scene system during destruction.");
        AZStd::shared_ptr<AzFramework::Scene> mainScene = sceneSystem->GetScene(SCENE_NAME);
        // This should never happen unless scene creation has changed.
        AZ_Assert(mainScene, "Main scenes missing during system component destruction");
        mainScene->UnsetSubsystem(m_scene);
        mainScene->UnsetSubsystem(m_entityContext.get());
        sceneSystem->RemoveScene(SCENE_NAME);

        AZ::RPI::RPISystemInterface::Get()->UnregisterScene(m_scene);
        m_scene = nullptr;
    }

    AZStd::shared_ptr<OpenParticleEditorViewportInputController> OpenParticleViewportRenderer::GetController()
    {
        return m_viewportController;
    }

    void OpenParticleViewportRenderer::CreateRenderScene()
    {
        // Create and register a scene with feature processors defined in the viewport settings
        AZ::RPI::SceneDescriptor sceneDesc;
        sceneDesc.m_nameId = SCENE_NAME;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        const char* viewportSettingPath = VIEWPOINT_SETTING_PATH.data();
        bool sceneDescLoaded = settingsRegistry->GetObject(sceneDesc, viewportSettingPath);
        m_scene = AZ::RPI::Scene::CreateScene(sceneDesc);

        if (!sceneDescLoaded)
        {
            AZ_Warning("OpenParticleViewportRenderer", false, "Settings registry is missing the scene settings for this viewport, so all feature "
                                                            "processors will be enabled. To enable only a minimal set, add the specific list of "
                                                            " feature processors with a registry path of '%s'.", viewportSettingPath);
            m_scene->EnableAllFeatureProcessors();
        }

        // Bind m_defaultScene to the GameEntityContext's AzFramework::Scene
        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "OpenParticleViewportRenderer was unable to get the scene system during construction.");
        AZ::Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> createSceneOutcome =
            sceneSystem->CreateScene(SCENE_NAME);
        AZStd::shared_ptr<AzFramework::Scene> mainScene = createSceneOutcome.TakeValue();

        // This should never happen unless scene creation has changed.
        AZ_Assert(mainScene, "OpenParticle scenes missing during system component initialization");
        mainScene->SetSubsystem(m_scene);
        mainScene->SetSubsystem(m_entityContext.get());
    }

    void OpenParticleViewportRenderer::CreateAssetRenderPipeline()
    {
        // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene
        AZ::Data::Asset<AZ::RPI::AnyAsset> pipelineAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
            m_defaultPipelineAssetPath.c_str(), AZ::RPI::AssetUtils::TraceLevel::Error);
        m_renderPipeline = AZ::RPI::RenderPipeline::CreateRenderPipelineForWindow(pipelineAsset, *m_windowContext.get());
        pipelineAsset.Release();
        m_scene->AddRenderPipeline(m_renderPipeline);
    }

    void OpenParticleViewportRenderer::CreateBRDFTexturePipeline() const
    {
        // As part of our initialization we need to create the BRDF texture generation pipeline
        AZ::RPI::RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_mainViewTagName = "MainCamera";
        pipelineDesc.m_name = "BRDFTexturePipeline";
        pipelineDesc.m_rootPassTemplate = "BRDFTexturePipeline";
        pipelineDesc.m_executeOnce = true;

        AZ::RPI::RenderPipelinePtr brdfTexturePipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);
        m_scene->AddRenderPipeline(brdfTexturePipeline);
    }

    void OpenParticleViewportRenderer::CreateCamera(AzFramework::EntityContextId& entityContextId)
    {
        // Configure camera
        AzFramework::EntityContextRequestBus::EventResult(
            m_cameraEntity, entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "Cameraentity");
        AZ_Assert(m_cameraEntity != nullptr, "Failed to create camera entity.");

        // Add debug camera and controller components
        AZ::Debug::CameraComponentConfig cameraConfig(m_windowContext);
        cameraConfig.m_fovY = AZ::Constants::HalfPi;
        cameraConfig.m_depthNear = DepthNear;
        m_cameraComponent = m_cameraEntity->CreateComponent(azrtti_typeid<AZ::Debug::CameraComponent>());
        m_cameraComponent->SetConfiguration(cameraConfig);
        m_cameraEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        m_cameraEntity->Init();
        m_cameraEntity->Activate();

        // Connect camera to pipeline's default view after camera entity activated
        m_renderPipeline->SetDefaultViewFromEntity(m_cameraEntity->GetId());
    }

    void OpenParticleViewportRenderer::ActiveView()
    {
        m_active = true;
    }

    void OpenParticleViewportRenderer::DeactiveView()
    {
        m_active = false;
    }

    const OpenParticleSystem::CameraTransform& OpenParticleViewportRenderer::GetParticleEditorCameraTransform()
    {
        m_cameraTransform.m_valid = m_active;
        if (m_active)
        {
            AZ::TransformBus::EventResult(m_cameraTransform.m_transform, m_cameraEntity->GetId(), &AZ::TransformBus::Events::GetWorldTM);
        }
        return m_cameraTransform;
    }

    void OpenParticleViewportRenderer::CreateParticleEntity(AzFramework::EntityContextId& entityContextId)
    {
        // Create particle
        AzFramework::EntityContextRequestBus::EventResult(
            m_particleEntity, entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "ParticleEntity");
        m_particleEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        [[maybe_unused]] auto particleComponent = m_particleEntity->CreateComponent(azrtti_typeid<OpenParticle::EditorParticleComponent>());
        AZ_Assert(particleComponent, "particle component create failed");
        m_particleEntity->Init();
        m_particleEntity->Activate();
        AZ::TransformBus::Event(m_particleEntity->GetId(), &AZ::TransformBus::Events::SetLocalTranslation, AZ::Vector3(0.0f, 0.0f, 0.0f));
    }

    void OpenParticleViewportRenderer::CreatePostProcessEntity(AzFramework::EntityContextId& entityContextId)
    {
        // Configure tone mapper
        AzFramework::EntityContextRequestBus::EventResult(
            m_postProcessEntity, entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "postProcessEntity");
        AZ_Assert(m_postProcessEntity != nullptr, "Failed to create post process entity.");

        m_postProcessEntity->CreateComponent(AZ::Render::PostFxLayerComponentTypeId);
        m_postProcessEntity->CreateComponent(AZ::Render::ExposureControlComponentTypeId);
        m_postProcessEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        m_postProcessEntity->Init();
        m_postProcessEntity->Activate();
    }

    void OpenParticleViewportRenderer::CreateGrid(AzFramework::EntityContextId& entityContextId)
    {
        // Create grid
        AzFramework::EntityContextRequestBus::EventResult(
            m_gridEntity, entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "ViewportGrid");
        AZ_Assert(m_gridEntity != nullptr, "Failed to create grid entity.");

        AZ::Render::GridComponentConfig gridConfig;
        gridConfig.m_gridSize = 4.0f;
        gridConfig.m_axisColor = AZ::Color(0.1f, 0.1f, 0.1f, 1.0f);
        gridConfig.m_primaryColor = AZ::Color(0.1f, 0.1f, 0.1f, 1.0f);
        gridConfig.m_secondaryColor = AZ::Color(0.1f, 0.1f, 0.1f, 1.0f);
        auto gridComponent = m_gridEntity->CreateComponent(AZ::Render::GridComponentTypeId);
        gridComponent->SetConfiguration(gridConfig);

        m_gridEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        m_gridEntity->Init();
        m_gridEntity->Activate();
    }

    void OpenParticleViewportRenderer::InitSkybox()
    {
        // Init Skybox
        m_skyboxFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
        m_skyboxFeatureProcessor->Enable(true);
        m_skyboxFeatureProcessor->SetSkyboxMode(AZ::Render::SkyBoxMode::Cubemap);
    }

    void OpenParticleViewportRenderer::ConnectEBus()
    {
        ParticleDocumentNotifyBus::Handler::BusConnect();
        OpenParticleViewportNotificationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_cameraEntity->GetId());
        OpenParticleSystem::ParticleEditorRequestBus::Handler::BusConnect();
    }

    void OpenParticleViewportRenderer::OnDocumentOpened(AZ::Data::Asset<OpenParticle::ParticleAsset> particleAsset, [[maybe_unused]] AZStd::string particleAssetPath)
    {
        OpenParticle::EditorParticleRequestBus::Event(m_particleEntity->GetId(), &OpenParticle::EditorParticleRequest::SetParticleAsset, particleAsset, true);
    }

    void OpenParticleViewportRenderer::OnDocumentInvisible()
    {
        OpenParticle::ParticleRequestBus::Event(m_particleEntity->GetId(), &OpenParticle::ParticleRequest::Stop);
    }

    void OpenParticleViewportRenderer::OnLightingPresetSelected(AZStd::string preset)
    {
        if (preset.empty())
        {
            AZ_Warning("OpenParticleViewportRenderer", false, "Attempting to set invalid lighting preset.");
            return;
        }
        
        AZ::Render::ImageBasedLightFeatureProcessorInterface* iblFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::ImageBasedLightFeatureProcessorInterface>();
        AZ::Render::PostProcessFeatureProcessorInterface* postProcessFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::PostProcessFeatureProcessorInterface>();

        AZ::Render::ExposureControlSettingsInterface* exposureControlSettingInterface = postProcessFeatureProcessor->GetOrCreateSettingsInterface(m_postProcessEntity->GetId())->GetOrCreateExposureControlSettingsInterface();

        Camera::Configuration cameraConfig;
        Camera::CameraRequestBus::EventResult(cameraConfig, m_cameraEntity->GetId(), &Camera::CameraRequestBus::Events::GetCameraConfiguration);

        bool enableAlternateSkybox = false;
        OpenParticleViewportRequestBus::BroadcastResult(
            enableAlternateSkybox, &OpenParticleViewportRequestBus::Events::GetAlternateSkyboxEnabled);

        AZ::Render::LightingPresetPtr presetPtr;
        EBUS_EVENT_RESULT(presetPtr, OpenParticleViewportRequestBus, GetLightingPresetByName, preset);
        if (presetPtr)
        {
            presetPtr->ApplyLightingPreset(
                iblFeatureProcessor, m_skyboxFeatureProcessor, exposureControlSettingInterface, m_directionalLightFeatureProcessor,
                cameraConfig, m_lightHandles, enableAlternateSkybox);
        }
    }

    void OpenParticleViewportRenderer::OnGridEnabledChanged(bool enable)
    {
        if (m_gridEntity)
        {
            if (enable && m_gridEntity->GetState() == AZ::Entity::State::Init)
            {
                m_gridEntity->Activate();
            }
            else if (!enable && m_gridEntity->GetState() == AZ::Entity::State::Active)
            {
                m_gridEntity->Deactivate();
            }
        }
    }

    void OpenParticleViewportRenderer::OnAlternateSkyboxEnabledChanged(bool enable)
    {
        AZ_UNUSED(enable);
        AZStd::string selectedPreset;
        OpenParticleViewportRequestBus::BroadcastResult(selectedPreset, &OpenParticleViewportRequestBus::Events::GetLightingPresetSelection);
        OnLightingPresetSelected(selectedPreset);
    }

    void OpenParticleViewportRenderer::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_initialized)
        {
            AzFramework::CameraState cameraState;
            OpenParticleViewportWidgetRequestsBus::BroadcastResult(cameraState, &OpenParticleViewportWidgetRequestsBus::Handler::CameraState);
            auto viewportSize = cameraState.m_viewportSize;
            auto dimensions = m_windowContext->GetSwapChain()->GetDescriptor().m_dimensions;
            if (static_cast<AZ::u32>(viewportSize.m_width) != dimensions.m_imageWidth)
            {
                m_initialized = true;
                auto swapChain = m_windowContext->GetSwapChain();
                AZ::RHI::SwapChainDimensions newDimensions = dimensions;
                newDimensions.m_imageWidth = viewportSize.m_width;
                newDimensions.m_imageHeight = viewportSize.m_height;
                AZ_Printf("OpenParticleViewportRenderer", "Resizing swapchain from (%d, %d) to (%d, %d)",
                    dimensions.m_imageWidth, dimensions.m_imageHeight, newDimensions.m_imageWidth, newDimensions.m_imageHeight);
                swapChain->Resize(newDimensions);
                return;
            }
        }
        m_renderPipeline->AddToRenderTickOnce();

        m_performanceMonitor.GatherMetrics();
        DrawPerformanceMonitorInfo();
        DrawAxisGizmo();
    }

    void OpenParticleViewportRenderer::DrawAxisGizmo() const
    {
        AZ::RPI::AuxGeomDrawPtr auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(m_scene);
        if (auxGeom == nullptr)
        {
            return;
        }

        // get the editor cameras current orientation
        AzFramework::CameraState editorCameraState;
        OpenParticleViewportWidgetRequestsBus::BroadcastResult(editorCameraState, &OpenParticleViewportWidgetRequestsBus::Handler::CameraState);
        AZ::Transform cameraTransform;
        AZ::TransformBus::EventResult(cameraTransform, m_cameraEntity->GetId(), &AZ::TransformBus::Events::GetLocalTM);
        editorCameraState = AzFramework::CreateDefaultCamera(cameraTransform, editorCameraState.m_viewportSize);

        const AZ::Matrix3x3& editorCameraOrientation = AZ::Matrix3x3::CreateFromMatrix3x4(AzFramework::CameraTransform(editorCameraState));

        // create a gizmo camera transform about the origin matching the orientation of the editor camera
        // (10 units back in the y axis to produce an orbit effect)
        const AZ::Transform gizmoCameraOffset = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(10.0f));
        const AZ::Transform gizmoCameraTransform = AZ::Transform::CreateFromMatrix3x3(editorCameraOrientation) * gizmoCameraOffset;
        const AzFramework::CameraState gizmoCameraState =
            AzFramework::CreateDefaultCamera(gizmoCameraTransform, editorCameraState.m_viewportSize);

        // cache the gizmo camera view and projection for world-to-screen calculations
        const auto cameraView = AzFramework::CameraView(gizmoCameraState);
        const auto cameraProjection = AzFramework::CameraProjection(gizmoCameraState);

        // screen space offset to move the 2d gizmo around
        const AZ::Vector2 screenOffset = AZ::Vector2(0.045f, 0.9f) - AZ::Vector2(0.5f, 0.5f);

        // map from a position in world space (relative to the the gizmo camera near the origin) to a position in
        // screen space
        const auto calculateGizmoAxis = [&cameraView, &cameraProjection, &screenOffset](const AZ::Vector3& axis)
        {
            auto result = AZ::Vector2(AzFramework::WorldToScreenNdc(axis, cameraView, cameraProjection));
            result.SetY(1.0f - result.GetY());
            return result + screenOffset;
        };

        // get all important axis positions in screen space
        const float lineLength = 0.7f;
        const auto gizmoStart = calculateGizmoAxis(AZ::Vector3::CreateZero());
        const auto gizmoEndAxisX = calculateGizmoAxis(-AZ::Vector3::CreateAxisX() * lineLength);
        const auto gizmoEndAxisY = calculateGizmoAxis(-AZ::Vector3::CreateAxisY() * lineLength);
        const auto gizmoEndAxisZ = calculateGizmoAxis(-AZ::Vector3::CreateAxisZ() * lineLength);

        const AZ::Vector2 gizmoAxisX = gizmoEndAxisX - gizmoStart;
        const AZ::Vector2 gizmoAxisY = gizmoEndAxisY - gizmoStart;
        const AZ::Vector2 gizmoAxisZ = gizmoEndAxisZ - gizmoStart;

        // draw the axes of the gizmo
        DrawLine2d(auxGeom, gizmoStart, gizmoEndAxisX, AZ::Colors::Red, 1.0);
        DrawLine2d(auxGeom, gizmoStart, gizmoEndAxisY, AZ::Colors::Green, 1.0);
        DrawLine2d(auxGeom, gizmoStart, gizmoEndAxisZ, AZ::Colors::Blue, 1.0);

        AzFramework::ViewportId viewportId;
        OpenParticleViewportWidgetRequestsBus::BroadcastResult(viewportId, &OpenParticleViewportWidgetRequestsBus::Handler::ViewportId);
        const float labelOffset = 1.15f;
        const auto viewportSize = AzFramework::Vector2FromScreenSize(editorCameraState.m_viewportSize);

        const auto labelXScreenPosition = (gizmoStart + (gizmoAxisX * labelOffset)) * viewportSize;
        const auto labelYScreenPosition = (gizmoStart + (gizmoAxisY * labelOffset)) * viewportSize;
        const auto labelZScreenPosition = (gizmoStart + (gizmoAxisZ * labelOffset)) * viewportSize;

        const float labelSize = 0.7f;
        Draw2dTextLabel(labelXScreenPosition.GetX(), labelXScreenPosition.GetY(), labelSize, "X", true, AZ::Colors::White);
        Draw2dTextLabel(labelYScreenPosition.GetX(), labelYScreenPosition.GetY(), labelSize, "Y", true, AZ::Colors::White);
        Draw2dTextLabel(labelZScreenPosition.GetX(), labelZScreenPosition.GetY(), labelSize, "Z", true, AZ::Colors::White);
    }

    void OpenParticleViewportRenderer::DrawPerformanceMonitorInfo()
    {
        const float x = 60.0f;
        const float y = 10.0f;
        const float fontSize = 0.6f;
        const PerformanceMetrics& metrics = m_performanceMonitor.GetMetrics();
        float frameRate = 0.0f;
        if (metrics.m_cpuFrameTimeMs > 0.0 || metrics.m_gpuFrameTimeMs > 0.0)
        {
            frameRate = 1000.0f / static_cast<float>(metrics.m_cpuFrameTimeMs + metrics.m_gpuFrameTimeMs);
        }
        AZStd::string str = AZStd::string::format("FPS %.1f", frameRate);
        Draw2dTextLabel(x, y, fontSize, str.c_str(), true, AZ::Colors::Yellow);
    }

    void OpenParticleViewportRenderer::Draw2dTextLabel(float x, float y, float size, const char* text, bool center, const AZ::Color& color) const
    {
        // abort draw if draw is invalid or font query interface is missing.
        if (!text || size == 0.0f || !AZ::Interface<AzFramework::FontQueryInterface>::Get())
        {
            return;
        }

        AzFramework::FontDrawInterface* fontDrawInterface =
            AZ::Interface<AzFramework::FontQueryInterface>::Get()->GetDefaultFontDrawInterface();
        // abort draw if font draw interface is missing
        if (!fontDrawInterface)
        {
            return;
        }
        AzFramework::ViewportId viewportId;
        OpenParticleViewportWidgetRequestsBus::BroadcastResult(viewportId, &OpenParticleViewportWidgetRequestsBus::Handler::ViewportId);

        // if 2d draw need to project pos to screen first
        AzFramework::TextDrawParameters params;

        params.m_drawViewportId = viewportId;
        params.m_position = AZ::Vector3(x, y, 1.0f);
        params.m_color = color; // m_rendState.m_color;
        params.m_scale = AZ::Vector2(size);
        params.m_hAlign = center ? AzFramework::TextHorizontalAlignment::Center
                                 : AzFramework::TextHorizontalAlignment::Left; //! Horizontal text alignment
        params.m_monospace = false; //! disable character proportional spacing
        params.m_depthTest = false; //! Test character against the depth buffer
        params.m_virtual800x600ScreenSize = false; //! Text placement and size are scaled in viewport pixel coordinates
        params.m_scaleWithWindow = false; //! Font gets bigger as the window gets bigger
        params.m_multiline = true; //! text respects ascii newline characters

        fontDrawInterface->DrawScreenAlignedText2d(params, text);
    }

    void OpenParticleViewportRenderer::DrawLine2d(
        AZ::RPI::AuxGeomDrawPtr auxGeomPtr, const AZ::Vector2& p1, const AZ::Vector2& p2, const AZ::Color& color, float z) const
    {
        if (auxGeomPtr == nullptr)
        {
            return;
        }

        AZ::Vector3 points[2];
        points[0] = AZ::Vector3(p1.GetX(), p1.GetY(), z);
        points[1] = AZ::Vector3(p2.GetX(), p2.GetY(), z);

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = 2;
        drawArgs.m_colors = &color;
        drawArgs.m_colorCount = 1;
        drawArgs.m_size = (AZ::u8)4.0f;
        drawArgs.m_depthTest = AZ::RPI::AuxGeomDraw::DepthTest::Off;
        drawArgs.m_viewProjectionOverrideIndex = auxGeomPtr->GetOrAdd2DViewProjOverride();
        auxGeomPtr->DrawLines(drawArgs);
    }

    void OpenParticleViewportRenderer::OnTransformChanged(const AZ::Transform&, const AZ::Transform&)
    {
        const AZ::EntityId* currentBusId = AZ::TransformNotificationBus::GetCurrentBusId();
        if (m_cameraEntity && currentBusId && *currentBusId == m_cameraEntity->GetId() && m_directionalLightFeatureProcessor)
        {
            auto transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                transform,
                m_cameraEntity->GetId(),
                &AZ::TransformBus::Events::GetWorldTM);
            for (const DirectionalLightHandle& id : m_lightHandles)
            {
                m_directionalLightFeatureProcessor->SetCameraTransform(
                    id, transform);
            }
        }
    }
}
