/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Integration/ActorComponentBus.h>
#include <Integration/Components/ActorComponent.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/Component/DebugCamera/CameraComponent.h>
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/ImageBasedLights/ImageBasedLightComponentConstants.h>

#include <EMStudio/AnimViewportRenderer.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>

namespace EMStudio
{
    AnimViewportRenderer::AnimViewportRenderer(AZ::RPI::ViewportContextPtr viewportContext, const RenderOptions* renderOptions)
        : m_windowContext(viewportContext->GetWindowContext())
        , m_renderOptions(renderOptions)
    {
        // Create a new entity context
        m_entityContext = AZStd::make_unique<AzFramework::EntityContext>();
        m_entityContext->InitContext();

        // Create the scene
        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "Unable to retrieve scene system.");
        AZ::Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> createSceneOutcome = sceneSystem->CreateScene("AnimViewport");
        AZ_Assert(createSceneOutcome, "%s", createSceneOutcome.GetError().data());
        m_frameworkScene = createSceneOutcome.TakeValue();
        m_frameworkScene->SetSubsystem<AzFramework::EntityContext::SceneStorageType>(m_entityContext.get());

        // Create and register a scene with feature processors defined in the viewport settings
        AZ::RPI::SceneDescriptor sceneDesc;
        sceneDesc.m_nameId = AZ::Name("AnimViewport");
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        const char* viewportSettingPath = "/O3DE/Editor/Viewport/Animation/Scene";
        bool sceneDescLoaded = settingsRegistry->GetObject(sceneDesc, viewportSettingPath);
        m_scene = AZ::RPI::Scene::CreateScene(sceneDesc);

        if (!sceneDescLoaded)
        {
            AZ_Warning("AnimViewportRenderer", false, "Settings registry is missing the scene settings for this viewport, so all feature processors will be enabled. "
                        "To enable only a minimal set, add the specific list of feature processors with a registry path of '%s'.", viewportSettingPath);
            m_scene->EnableAllFeatureProcessors();
        }

        // Link our RPI::Scene to the AzFramework::Scene
        m_frameworkScene->SetSubsystem(m_scene);

        AZStd::string pipelineAssetPath = "passes/MainRenderPipeline.azasset";
        AZ::RPI::XRRenderingInterface* xrSystem = AZ::RPI::RPISystemInterface::Get()->GetXRSystem();
        if (xrSystem)
        {
            // OpenXr uses low end render pipeline
            pipelineAssetPath = "passes/LowEndRenderPipeline.azasset";
        }

        AZStd::optional<AZ::RPI::RenderPipelineDescriptor> renderPipelineDesc =
            AZ::RPI::GetRenderPipelineDescriptorFromAsset(pipelineAssetPath.c_str(), AZStd::string::format("_%i", viewportContext->GetId()));
        AZ_Assert(renderPipelineDesc.has_value(), "Invalid render pipeline descriptor from asset %s", pipelineAssetPath.c_str());

        const AZ::RHI::MultisampleState multiSampleState = AZ::RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();
        renderPipelineDesc.value().m_renderSettings.m_multisampleState = multiSampleState;
        AZ_Printf("AnimViewportRenderer", "Animation viewport renderer starting with multi sample %d", multiSampleState.m_samples);
        
        m_renderPipeline = AZ::RPI::RenderPipeline::CreateRenderPipelineForWindow(renderPipelineDesc.value(), *m_windowContext.get());
        m_scene->AddRenderPipeline(m_renderPipeline);
        m_renderPipeline->SetDefaultView(viewportContext->GetDefaultView());

        // Currently the scene has to be activated after render pipeline was added so some feature processors (i.e. imgui) can be
        // initialized properly with pipeline's pass information.
        m_scene->Activate();
        AZ::RPI::RPISystemInterface::Get()->RegisterScene(m_scene);
        AzFramework::EntityContextId entityContextId = m_entityContext->GetContextId();

        // Get the FeatureProcessors
        m_meshFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::MeshFeatureProcessorInterface>();

        // Configure tone mapper
        AzFramework::EntityContextRequestBus::EventResult(
            m_postProcessEntity, entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "postProcessEntity");
        AZ_Assert(m_postProcessEntity != nullptr, "Failed to create post process entity.");

        m_postProcessEntity->CreateComponent(AZ::Render::PostFxLayerComponentTypeId);
        m_postProcessEntity->CreateComponent(AZ::Render::ExposureControlComponentTypeId);
        m_postProcessEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        m_postProcessEntity->Init();
        m_postProcessEntity->Activate();

        // Init directional light processor
        m_directionalLightFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DirectionalLightFeatureProcessorInterface>();

        // Init display mapper processor
        m_displayMapperFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::DisplayMapperFeatureProcessorInterface>();

        // Init Skybox
        m_skyboxFeatureProcessor = m_scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
        m_skyboxFeatureProcessor->Enable(true);
        m_skyboxFeatureProcessor->SetSkyboxMode(AZ::Render::SkyBoxMode::Cubemap);

        // Create IBL
        AzFramework::EntityContextRequestBus::EventResult(
            m_iblEntity, entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "IblEntity");
        AZ_Assert(m_iblEntity != nullptr, "Failed to create ibl entity.");

        m_iblEntity->CreateComponent(AZ::Render::ImageBasedLightComponentTypeId);
        m_iblEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        m_iblEntity->Init();
        m_iblEntity->Activate();

        // Load light preset
        AZ::Data::Asset<AZ::RPI::AnyAsset> lightingPresetAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
            "lightingpresets/default.lightingpreset.azasset", AZ::RPI::AssetUtils::TraceLevel::Warning);
        const AZ::Render::LightingPreset* preset = lightingPresetAsset->GetDataAs<AZ::Render::LightingPreset>();
        SetLightingPreset(preset);

        // Create the ground plane
        AzFramework::EntityContextRequestBus::EventResult(
            m_groundEntity, entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "ViewportModel");
        AZ_Assert(m_groundEntity != nullptr, "Failed to create model entity.");

        m_groundEntity->CreateComponent(AZ::Render::MeshComponentTypeId);
        m_groundEntity->CreateComponent(AZ::Render::MaterialComponentTypeId);
        m_groundEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        m_groundEntity->Init();
        m_groundEntity->Activate();

        Reinit();
    }

    AnimViewportRenderer::~AnimViewportRenderer()
    {
        // Destroy all the entity we created.
        m_entityContext->DestroyEntity(m_iblEntity);
        m_entityContext->DestroyEntity(m_postProcessEntity);
        m_entityContext->DestroyEntity(m_groundEntity);
        for (AZ::Entity* entity : m_actorEntities)
        {
            m_entityContext->DestroyEntity(entity);
        }
        m_actorEntities.clear();
        m_entityContext->DestroyContext();

        for (AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle& handle : m_lightHandles)
        {
            m_directionalLightFeatureProcessor->ReleaseLight(handle);
        }
        m_lightHandles.clear();

        m_frameworkScene->UnsetSubsystem(m_scene);

        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "AtomViewportRenderer was unable to get the scene system during destruction.");
        bool removeSuccess = sceneSystem->RemoveScene("AnimViewport");
        if (!removeSuccess)
        {
            AZ_Assert(false, "AtomViewportRenderer should be removed.");
        }

        AZ::RPI::RPISystemInterface::Get()->UnregisterScene(m_scene);
        m_scene = nullptr;
    }

    void AnimViewportRenderer::Reinit()
    {
        ReinitActorEntities();
        ResetEnvironment();
    }

    void AnimViewportRenderer::MoveActorEntitiesToOrigin()
    {
        for (AZ::Entity* entity : m_actorEntities)
        {
            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateIdentity());
        }
    }

    AZ::Vector3 AnimViewportRenderer::GetCharacterCenter() const
    {
        AZ::Vector3 result = AZ::Vector3::CreateZero();
        if (!m_actorEntities.empty())
        {
            // Find the actor instance and calculate the center from aabb.
            EMotionFX::Integration::ActorComponent* actorComponent =
                m_actorEntities[0]->FindComponent<EMotionFX::Integration::ActorComponent>();
            EMotionFX::ActorInstance* actorInstance = actorComponent->GetActorInstance();
            if (actorInstance)
            {
                result = actorInstance->GetAabb().GetCenter();
            }
        }

        return result;
    }

    void AnimViewportRenderer::UpdateActorRenderFlag(EMotionFX::ActorRenderFlags renderFlags)
    {
        for (AZ::Entity* entity : m_actorEntities)
        {
            EMotionFX::Integration::ActorComponent* actorComponent = entity->FindComponent<EMotionFX::Integration::ActorComponent>();
            if (!actorComponent)
            {
                AZ_Assert(false, "Found entity without actor component in the actor entity list.");
                continue;
            }
            actorComponent->SetRenderFlag(renderFlags);
        }
    }

    AZStd::shared_ptr<AzFramework::Scene> AnimViewportRenderer::GetFrameworkScene() const
    {
        return m_frameworkScene;
    }

    AZ::EntityId AnimViewportRenderer::GetEntityId() const
    {
        if (m_actorEntities.empty())
        {
            return AZ::EntityId();
        }
        return m_actorEntities[0]->GetId();
    }

    AzFramework::EntityContextId AnimViewportRenderer::GetEntityContextId() const
    {
        return m_entityContext->GetContextId();
    }

    void AnimViewportRenderer::UpdateGroundplane()
    {
        AZ::Vector3 groundPos;
        AZ::TransformBus::EventResult(groundPos, m_groundEntity->GetId(), &AZ::TransformBus::Events::GetWorldTranslation);

        const AZ::Vector3 characterPos = GetCharacterCenter();
        const float tileOffsetX = AZStd::fmod(characterPos.GetX(), TileSize);
        const float tileOffsetY = AZStd::fmod(characterPos.GetY(), TileSize);
        const AZ::Vector3 newGroundPos(characterPos.GetX() - tileOffsetX, characterPos.GetY() - tileOffsetY, groundPos.GetZ());

        AZ::TransformBus::Event(m_groundEntity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, newGroundPos);
    }

    void AnimViewportRenderer::ResetEnvironment()
    {
        // Reset environment
        AZ::Transform iblTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_iblEntity->GetId(), &AZ::TransformBus::Events::SetLocalTM, iblTransform);

        const AZ::Matrix4x4 rotationMatrix = AZ::Matrix4x4::CreateIdentity();
        auto skyBoxFeatureProcessorInterface = m_scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
        skyBoxFeatureProcessorInterface->SetCubemapRotationMatrix(rotationMatrix);

        // Reset ground entity
        AZ::Transform identityTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_groundEntity->GetId(), &AZ::TransformBus::Events::SetLocalTM, identityTransform);

        auto modelAsset = AZ::RPI::AssetUtils::GetAssetByProductPath<AZ::RPI::ModelAsset>(
            "objects/groudplane/groundplane_512x512m.fbx.azmodel", AZ::RPI::AssetUtils::TraceLevel::Assert);
        AZ::Render::MeshComponentRequestBus::Event(
            m_groundEntity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::SetModelAsset, modelAsset);

        // Reset actor position
        for (AZ::Entity* entity : m_actorEntities)
        {
            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetLocalTM, identityTransform);
        }
    }

    void AnimViewportRenderer::ReinitActorEntities()
    {
        // 1. Destroy all the entities that do not point to any actorAsset anymore.
        AZStd::set<AZ::Data::AssetId> assetLookup;
        AzFramework::EntityContext* entityContext = m_entityContext.get();
        const size_t numActors = EMotionFX::GetActorManager().GetNumActors();
        for (size_t i = 0; i < numActors; ++i)
        {
            assetLookup.emplace(EMotionFX::GetActorManager().GetActorAsset(i).GetId());
        }
        m_actorEntities.erase(
            AZStd::remove_if(
                m_actorEntities.begin(), m_actorEntities.end(),
                [&assetLookup, entityContext](AZ::Entity* entity)
                {
                    EMotionFX::Integration::ActorComponent* actorComponent =
                        entity->FindComponent<EMotionFX::Integration::ActorComponent>();
                    if (assetLookup.find(actorComponent->GetActorAsset().GetId()) == assetLookup.end())
                    {
                        entityContext->DestroyEntity(entity);
                        return true;
                    }
                    return false;
                }),
            m_actorEntities.end());

        // 2. Create an entity for every actorAsset stored in actor manager.
        for (size_t i = 0; i < numActors; ++i)
        {
            AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset = EMotionFX::GetActorManager().GetActorAsset(i);
            if (!actorAsset->IsReady())
            {
                continue;
            }

            AZ::Entity* entity = FindActorEntity(actorAsset);
            if (!entity)
            {
                m_actorEntities.emplace_back(CreateActorEntity(actorAsset));
            }
        }
    }

    AZ::Entity* AnimViewportRenderer::FindActorEntity(AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset) const
    {
        const auto foundEntity = AZStd::find_if(
            begin(m_actorEntities), end(m_actorEntities),
            [match = actorAsset](const AZ::Entity* entity)
            {
                EMotionFX::Integration::ActorComponent* actorComponent = entity->FindComponent<EMotionFX::Integration::ActorComponent>();
                return actorComponent->GetActorAsset() == match;
            });
        return foundEntity != end(m_actorEntities) ? (*foundEntity) : nullptr;
    }

    AZ::Entity* AnimViewportRenderer::CreateActorEntity(AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset)
    {
        AZ::Entity* actorEntity = m_entityContext->CreateEntity(actorAsset->GetActor()->GetName());
        actorEntity->CreateComponent(azrtti_typeid<EMotionFX::Integration::ActorComponent>());
        actorEntity->CreateComponent(AZ::Render::MaterialComponentTypeId);
        actorEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        actorEntity->Init();
        actorEntity->Activate();

        EMotionFX::Integration::ActorComponent* actorComponent = actorEntity->FindComponent<EMotionFX::Integration::ActorComponent>();
        actorComponent->SetActorAsset(actorAsset);

        // Since this entity belongs to the animation editor, we need to set the isOwnByRuntime flag to false.
        actorComponent->GetActorInstance()->SetIsOwnedByRuntime(false);
        // Selet the actor instance in the command manager after it has been created.
        AZStd::string outResult;
        EMStudioManager::GetInstance()->GetCommandManager()->ExecuteCommandInsideCommand(
            AZStd::string::format("Select -actorInstanceID %i", actorComponent->GetActorInstance()->GetID()).c_str(), outResult);

        return actorEntity;
    }

    void AnimViewportRenderer::SetLightingPreset(const AZ::Render::LightingPreset* preset)
    {
        if (!preset)
        {
            AZ_Warning("AnimViewportRenderer", false, "Attempting to set invalid lighting preset.");
            return;
        }

        AZ::Render::ImageBasedLightFeatureProcessorInterface* iblFeatureProcessor =
            m_scene->GetFeatureProcessor<AZ::Render::ImageBasedLightFeatureProcessorInterface>();
        AZ::Render::PostProcessFeatureProcessorInterface* postProcessFeatureProcessor =
            m_scene->GetFeatureProcessor<AZ::Render::PostProcessFeatureProcessorInterface>();
        AZ::Render::ExposureControlSettingsInterface* exposureControlSettingInterface =
            postProcessFeatureProcessor->GetOrCreateSettingsInterface(m_postProcessEntity->GetId())
                ->GetOrCreateExposureControlSettingsInterface();

        Camera::Configuration cameraConfig;
        cameraConfig.m_fovRadians = AZ::DegToRad(m_renderOptions->GetFOV());
        cameraConfig.m_nearClipDistance = m_renderOptions->GetNearClipPlaneDistance();
        cameraConfig.m_farClipDistance = m_renderOptions->GetFarClipPlaneDistance();
        cameraConfig.m_frustumWidth = DefaultFrustumDimension;
        cameraConfig.m_frustumHeight = DefaultFrustumDimension;

        preset->ApplyLightingPreset(
            iblFeatureProcessor, m_skyboxFeatureProcessor, exposureControlSettingInterface, m_directionalLightFeatureProcessor,
            cameraConfig, m_lightHandles, false);
    }

    AZ::RPI::SceneId AnimViewportRenderer::GetRenderSceneId() const
    {
        return m_scene->GetId();
    }

    const AZStd::vector<AZ::Entity*>& AnimViewportRenderer::GetActorEntities() const
    {
        return m_actorEntities;
    }
} // namespace EMStudio
