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
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Grid/GridComponentConfig.h>
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


namespace EMStudio
{
    static constexpr float DepthNear = 0.01f;

    AnimViewportRenderer::AnimViewportRenderer(AZ::RPI::ViewportContextPtr viewportContext)
        : m_windowContext(viewportContext->GetWindowContext())
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

        // Create and register a scene with all available feature processors
        AZ::RPI::SceneDescriptor sceneDesc;
        m_scene = AZ::RPI::Scene::CreateScene(sceneDesc);
        m_scene->EnableAllFeatureProcessors();

        // Link our RPI::Scene to the AzFramework::Scene
        m_frameworkScene->SetSubsystem(m_scene);

        // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene
        AZStd::string defaultPipelineAssetPath = "passes/MainRenderPipeline.azasset";
        AZ::Data::Asset<AZ::RPI::AnyAsset> pipelineAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
            defaultPipelineAssetPath.c_str(), AZ::RPI::AssetUtils::TraceLevel::Error);
        m_renderPipeline = AZ::RPI::RenderPipeline::CreateRenderPipelineForWindow(pipelineAsset, *m_windowContext.get());
        pipelineAsset.Release();
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

        // Create grid
        AzFramework::EntityContextRequestBus::EventResult(
            m_gridEntity, entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "ViewportGrid");
        AZ_Assert(m_gridEntity != nullptr, "Failed to create grid entity.");

        AZ::Render::GridComponentConfig gridConfig;
        gridConfig.m_gridSize = 4.0f;
        gridConfig.m_axisColor = AZ::Color(0.5f, 0.5f, 0.5f, 1.0f);
        gridConfig.m_primaryColor = AZ::Color(0.3f, 0.3f, 0.3f, 1.0f);
        gridConfig.m_secondaryColor = AZ::Color(0.5f, 0.1f, 0.1f, 1.0f);
        auto gridComponent = m_gridEntity->CreateComponent(AZ::Render::GridComponentTypeId);
        gridComponent->SetConfiguration(gridConfig);

        m_gridEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        m_gridEntity->Init();
        m_gridEntity->Activate();

        Reinit();
    }

    AnimViewportRenderer::~AnimViewportRenderer()
    {
        // Destroy all the entity we created.
        m_entityContext->DestroyEntity(m_iblEntity);
        m_entityContext->DestroyEntity(m_postProcessEntity);
        m_entityContext->DestroyEntity(m_gridEntity);
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

    AZ::Vector3 AnimViewportRenderer::GetCharacterCenter() const
    {
        AZ::Vector3 result = AZ::Vector3::CreateZero();
        if (!m_actorEntities.empty())
        {
            // Find the actor instance and calculate the center from aabb.
            AZ::Vector3 actorCenter = AZ::Vector3::CreateZero();
            EMotionFX::Integration::ActorComponent* actorComponent =
                m_actorEntities[0]->FindComponent<EMotionFX::Integration::ActorComponent>();
            EMotionFX::ActorInstance* actorInstance = actorComponent->GetActorInstance();
            if (actorInstance)
            {
                actorCenter += actorInstance->GetAabb().GetCenter();
            }
            
            // Just return the position of the first entity.
            AZ::Transform worldTransform;
            AZ::TransformBus::EventResult(worldTransform, m_actorEntities[0]->GetId(), &AZ::TransformBus::Events::GetWorldTM);
            result = worldTransform.GetTranslation();
            result += actorCenter;
        }

        return result;
    }

    void AnimViewportRenderer::UpdateActorRenderFlag(EMotionFX::ActorRenderFlagBitset renderFlags)
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

    void AnimViewportRenderer::ResetEnvironment()
    {
        // Reset environment
        AZ::Transform iblTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_iblEntity->GetId(), &AZ::TransformBus::Events::SetLocalTM, iblTransform);

        const AZ::Matrix4x4 rotationMatrix = AZ::Matrix4x4::CreateIdentity();
        AZ::RPI::ScenePtr scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
        auto skyBoxFeatureProcessorInterface = scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
        skyBoxFeatureProcessorInterface->SetCubemapRotationMatrix(rotationMatrix);
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
        cameraConfig.m_fovRadians = AZ::Constants::HalfPi;
        cameraConfig.m_nearClipDistance = DepthNear;

        preset->ApplyLightingPreset(
            iblFeatureProcessor, m_skyboxFeatureProcessor, exposureControlSettingInterface, m_directionalLightFeatureProcessor,
            cameraConfig, m_lightHandles, nullptr, AZ::RPI::MaterialPropertyIndex::Null, false);
    }
} // namespace EMStudio
