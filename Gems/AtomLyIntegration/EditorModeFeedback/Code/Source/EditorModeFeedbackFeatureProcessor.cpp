/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackFeatureProcessor.h>
#include <Pass/State/FocusedEntityState.h>
#include <Pass/State/SelectedEntityState.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/ModelLod.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/Utils/Utils.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshHandleStateBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            [[maybe_unused]] const char* const Window = "EditorModeFeedback";
            constexpr const char* ViewModePassTemplatesAssetPath = "Passes/ViewModePassTemplates.azasset";
            constexpr const char* ViewModeMaterialProductPath = "shaders/viewmode/viewmode.azmaterial";
            constexpr const char* ViewModeSrgShaderProductPath = "shaders/viewmode/viewmodewireframe.azshader";

            bool g_viewModePassTemplatesLoaded = false;
        }

        // Creates the material for the mask pass shader
        static Data::Instance<RPI::Material> CreateMaskMaterial()
        {
            const AZStd::string path = "shaders/editormodemask.azmaterial";
            const auto materialAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::MaterialAsset>(path);
            const auto maskMaterial = RPI::Material::FindOrCreate(materialAsset);
            return maskMaterial;
        }

        void EditorModeFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<EditorModeFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(1);
            }
        }

        void EditorModeFeatureProcessor::Activate()
        {
            EnableSceneNotification();

            EditorStateList editorStates;
            editorStates.push_back(AZStd::make_unique<FocusedEntityState>(
                [this]
                {
                    return GetWorldId();
                }));
            editorStates.push_back(AZStd::make_unique<SelectedEntityState>());
            m_editorStatePassSystem = AZStd::make_unique<EditorStatePassSystem>(AZStd::move(editorStates));
            AZ::TickBus::Handler::BusConnect();
        }

        AzFramework::EntityContextId EditorModeFeatureProcessor::GetWorldId()
        {
            if (!m_worldId.IsNull())
            {
                return m_worldId;
            }

            // The framework scene owning this render scene names the world rendered here.
            AZStd::shared_ptr<AzFramework::Scene> owningScene;
            if (auto* sceneSystem = AzFramework::SceneSystemInterface::Get())
            {
                sceneSystem->IterateActiveScenes(
                    [this, &owningScene](const AZStd::shared_ptr<AzFramework::Scene>& scene)
                    {
                        auto* renderScene = scene->FindSubsystemInScene<RPI::ScenePtr>();
                        const bool owns = renderScene && renderScene->get() == GetParentScene();
                        if (owns)
                        {
                            owningScene = scene;
                        }
                        return !owns;
                    });
            }
            if (!owningScene)
            {
                return AzToolsFramework::GetEntityContextId();
            }

            auto worldId = AzFramework::EntityContextId::CreateNull();
            if (auto* entityContext = owningScene->FindSubsystemInScene<AzFramework::EntityContext::SceneStorageType>();
                entityContext && *entityContext)
            {
                worldId = (*entityContext)->GetContextId();
            }

            // Only registered worlds (and world 0 itself) resolve to a scene; anything else (the
            // main scene carries the game context) is world 0, the editor context.
            m_worldId = AzToolsFramework::GetEntityContextId();
            if (!worldId.IsNull())
            {
                AZStd::shared_ptr<AzFramework::Scene> worldScene;
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                    worldScene, &AzToolsFramework::EditorEntityContextRequests::GetWorldScene, worldId);
                if (worldScene)
                {
                    m_worldId = worldId;
                }
            }
            return m_worldId;
        }

        void EditorModeFeatureProcessor::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            m_editorStatePassSystem.reset();
            DisableSceneNotification();
        }

        void EditorModeFeatureProcessor::OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline,
            RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (!m_editorStatePassSystem)
            {
                return;
            }

            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added
                || changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                m_editorStatePassSystem->ConfigureStatePassesForPipeline(renderPipeline);
            }
            else if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                m_editorStatePassSystem->RemoveStatePassesForPipeline(renderPipeline);
            }
        }

        void EditorModeFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
        {
            if (!m_editorStatePassSystem)
            {
                return;
            }

            m_editorStatePassSystem->AddPassesToRenderPipeline(renderPipeline);

            if(!m_maskRenderers.empty())
            {
                return;
            }

            for (const auto& mask : m_editorStatePassSystem->GetMasks())
            {
                // Emplaces the mask key and mask renderer value in place for the mask renderers map
                m_maskRenderers.emplace(
                    AZStd::piecewise_construct, AZStd::forward_as_tuple(mask), AZStd::forward_as_tuple(mask));
            }
        }

        void EditorModeFeatureProcessor::Render([[maybe_unused]] const RenderPacket& packet)
        {
            if (!m_editorStatePassSystem || !m_maskMaterial)
            {
                return;
            }

            const auto entityMaskMap = m_editorStatePassSystem->GetEntitiesForEditorStates();
            for (const auto& [mask, entities] : entityMaskMap)
            {
                if(auto it = m_maskRenderers.find(mask);
                    it != m_maskRenderers.end())
                {
                    // Every scene's feature processor sees the same editor state; each renders only its own entities.
                    auto sceneEntities = entities;
                    AZStd::erase_if(
                        sceneEntities,
                        [this](const AZ::EntityId& entityId)
                        {
                            return RPI::Scene::GetSceneForEntityId(entityId) != GetParentScene();
                        });
                    it->second.RenderMaskEntities(m_maskMaterial, sceneEntities);
                }
            }
        }

        void EditorModeFeatureProcessor::Simulate([[maybe_unused]] const SimulatePacket& packet)
        {
            if (!m_editorStatePassSystem)
            {
                return;
            }

            m_editorStatePassSystem->Update();
        }

        void EditorModeFeatureProcessor::OnTick(float, AZ::ScriptTimePoint)
        {
            // Attempt deferred loading of mask material until the asset is ready
            if (!m_maskMaterial)
            {
                m_maskMaterial = CreateMaskMaterial();
            }

            if (m_maskMaterial)
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        }

        const EditorStateBase* EditorModeFeatureProcessor::GetEditorState(EditorState editorState) const
        {
            return m_editorStatePassSystem ? m_editorStatePassSystem->GetEditorState(editorState) : nullptr;
        }

        void EditorModeFeatureProcessor::SetEnableRender(bool enableRender)
        {            
            if (!m_editorStatePassSystem)
            {
                return;
            }

            const auto templateName = Name(m_editorStatePassSystem->GetParentPassTemplateName());

            auto passFilter = AZ::RPI::PassFilter::CreateWithTemplateName(templateName, GetParentScene());
            AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter,  [enableRender](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    pass->SetEnabled(enableRender);
                    return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void ViewModeFeatureProcessor::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            serializeContext &&
                (serializeContext->Class<ViewModeFeatureProcessor, AZ::RPI::FeatureProcessor>()->Version(0), true);
        }

        void ViewModeFeatureProcessor::Activate()
        {
            auto* passSystem = AZ::RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "ViewModeFeatureProcessor activated before the pass system");

            (!g_viewModePassTemplatesLoaded && passSystem) &&
                (g_viewModePassTemplatesLoaded = passSystem->LoadPassTemplateMappings(ViewModePassTemplatesAssetPath), true);
            AZ_Error(
                Window, g_viewModePassTemplatesLoaded, "Failed to load '%s'; the viewport view modes are unavailable",
                ViewModePassTemplatesAssetPath);

            QueueAssetLoads();
            EnableSceneNotification();
        }

        void ViewModeFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            m_meshes.clear();
            m_pipelinePasses.clear();
            m_material = nullptr;
            m_materialAsset.Release();
            m_srgShaderAsset.Release();
        }

        void ViewModeFeatureProcessor::QueueAssetLoads()
        {
            const bool needMaterialAsset = !m_materialAsset.GetId().IsValid();
            AZ::Data::AssetId materialAssetId;
            needMaterialAsset &&
                (materialAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(
                     ViewModeMaterialProductPath, AZ::RPI::AssetUtils::TraceLevel::None),
                 true);
            (needMaterialAsset && materialAssetId.IsValid()) &&
                (m_materialAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(
                     materialAssetId, AZ::Data::AssetLoadBehavior::PreLoad),
                 true);

            const bool needSrgShaderAsset = !m_srgShaderAsset.GetId().IsValid();
            AZ::Data::AssetId srgShaderAssetId;
            needSrgShaderAsset &&
                (srgShaderAssetId = AZ::RPI::AssetUtils::GetAssetIdForProductPath(
                     ViewModeSrgShaderProductPath, AZ::RPI::AssetUtils::TraceLevel::None),
                 true);
            (needSrgShaderAsset && srgShaderAssetId.IsValid()) &&
                (m_srgShaderAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::ShaderAsset>(
                     srgShaderAssetId, AZ::Data::AssetLoadBehavior::PreLoad),
                 true);
        }

        bool ViewModeFeatureProcessor::EnsureAssets()
        {
            QueueAssetLoads();

            (!m_material && m_materialAsset.IsReady()) && (m_material = AZ::RPI::Material::FindOrCreate(m_materialAsset), true);

            return m_material && m_srgShaderAsset.IsReady();
        }

        void ViewModeFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
        {
            AZ_Assert(renderPipeline, "AddRenderPasses called with a null render pipeline");

            (!g_viewModePassTemplatesLoaded) &&
                (g_viewModePassTemplatesLoaded =
                     AZ::RPI::PassSystemInterface::Get()->LoadPassTemplateMappings(ViewModePassTemplatesAssetPath),
                 true);

            const bool defaultView = renderPipeline->GetViewType() == AZ::RPI::ViewType::Default;
            auto existingFilter =
                AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ViewModeWireframePassTemplate"), renderPipeline);
            const bool alreadyInjected = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(existingFilter) != nullptr;
            const bool anchorsPresent = renderPipeline->FindFirstPass(AZ::Name("PostProcessPass")) != nullptr &&
                renderPipeline->FindFirstPass(AZ::Name("DepthPrePass")) != nullptr;

            AZ_Warning(
                Window, !defaultView || alreadyInjected || anchorsPresent,
                "Pipeline '%s' has no PostProcessPass/DepthPrePass anchors; the viewport view modes are unavailable in it",
                renderPipeline->GetId().GetCStr());

            (defaultView && !alreadyInjected && anchorsPresent && g_viewModePassTemplatesLoaded) &&
                (InjectPasses(*renderPipeline), true);
        }

        void ViewModeFeatureProcessor::InjectPasses(AZ::RPI::RenderPipeline& renderPipeline)
        {
            auto* passSystem = AZ::RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "InjectPasses called without a pass system");

            AZ::RPI::PassRequest backgroundRequest;
            backgroundRequest.m_passName = AZ::Name("ViewModeBackgroundPass");
            backgroundRequest.m_templateName = AZ::Name("ViewModeBackgroundPassTemplate");
            backgroundRequest.m_passEnabled = false;
            backgroundRequest.AddInputConnection(AZ::RPI::PassConnection{
                AZ::Name("ColorInputOutput"), AZ::RPI::PassAttachmentRef{ AZ::Name("PostProcessPass"), AZ::Name("Output") } });
            AZ::RPI::Ptr<AZ::RPI::Pass> backgroundPass = passSystem->CreatePassFromRequest(&backgroundRequest);
            backgroundPass && (renderPipeline.AddPassAfter(backgroundPass, AZ::Name("PostProcessPass")), true);

            AZ::RPI::PassRequest wireframeHiddenRequest;
            wireframeHiddenRequest.m_passName = AZ::Name("ViewModeWireframeHiddenPass");
            wireframeHiddenRequest.m_templateName = AZ::Name("ViewModeWireframeHiddenPassTemplate");
            wireframeHiddenRequest.m_passEnabled = false;
            wireframeHiddenRequest.AddInputConnection(AZ::RPI::PassConnection{
                AZ::Name("ColorInputOutput"),
                AZ::RPI::PassAttachmentRef{ AZ::Name("ViewModeBackgroundPass"), AZ::Name("ColorInputOutput") } });
            wireframeHiddenRequest.AddInputConnection(AZ::RPI::PassConnection{
                AZ::Name("DepthInputOutput"), AZ::RPI::PassAttachmentRef{ AZ::Name("DepthPrePass"), AZ::Name("Depth") } });
            AZ::RPI::Ptr<AZ::RPI::Pass> wireframeHiddenPass;
            backgroundPass && (wireframeHiddenPass = passSystem->CreatePassFromRequest(&wireframeHiddenRequest), true);
            wireframeHiddenPass &&
                (renderPipeline.AddPassAfter(wireframeHiddenPass, AZ::Name("ViewModeBackgroundPass")), true);

            AZ::RPI::PassRequest wireframeRequest;
            wireframeRequest.m_passName = AZ::Name("ViewModeWireframePass");
            wireframeRequest.m_templateName = AZ::Name("ViewModeWireframePassTemplate");
            wireframeRequest.m_passEnabled = false;
            wireframeRequest.AddInputConnection(AZ::RPI::PassConnection{
                AZ::Name("ColorInputOutput"),
                AZ::RPI::PassAttachmentRef{ AZ::Name("ViewModeWireframeHiddenPass"), AZ::Name("ColorInputOutput") } });
            wireframeRequest.AddInputConnection(AZ::RPI::PassConnection{
                AZ::Name("DepthInputOutput"), AZ::RPI::PassAttachmentRef{ AZ::Name("DepthPrePass"), AZ::Name("Depth") } });
            AZ::RPI::Ptr<AZ::RPI::Pass> wireframePass;
            wireframeHiddenPass && (wireframePass = passSystem->CreatePassFromRequest(&wireframeRequest), true);
            wireframePass && (renderPipeline.AddPassAfter(wireframePass, AZ::Name("ViewModeWireframeHiddenPass")), true);

            AZ::RPI::PassRequest countRequest;
            countRequest.m_passName = AZ::Name("ViewModeOverdrawCountPass");
            countRequest.m_templateName = AZ::Name("ViewModeOverdrawCountPassTemplate");
            countRequest.m_passEnabled = false;
            countRequest.AddInputConnection(AZ::RPI::PassConnection{
                AZ::Name("DepthInputOutput"), AZ::RPI::PassAttachmentRef{ AZ::Name("DepthPrePass"), AZ::Name("Depth") } });
            AZ::RPI::Ptr<AZ::RPI::Pass> countPass;
            wireframePass && (countPass = passSystem->CreatePassFromRequest(&countRequest), true);
            countPass && (renderPipeline.AddPassAfter(countPass, AZ::Name("ViewModeWireframePass")), true);

            AZ::RPI::PassRequest resolveRequest;
            resolveRequest.m_passName = AZ::Name("ViewModeOverdrawResolvePass");
            resolveRequest.m_templateName = AZ::Name("ViewModeOverdrawResolvePassTemplate");
            resolveRequest.m_passEnabled = false;
            resolveRequest.AddInputConnection(AZ::RPI::PassConnection{
                AZ::Name("Count"), AZ::RPI::PassAttachmentRef{ AZ::Name("ViewModeOverdrawCountPass"), AZ::Name("Count") } });
            resolveRequest.AddInputConnection(AZ::RPI::PassConnection{
                AZ::Name("ColorInputOutput"),
                AZ::RPI::PassAttachmentRef{ AZ::Name("ViewModeWireframePass"), AZ::Name("ColorInputOutput") } });
            AZ::RPI::Ptr<AZ::RPI::Pass> resolvePass;
            countPass && (resolvePass = passSystem->CreatePassFromRequest(&resolveRequest), true);
            resolvePass && (renderPipeline.AddPassAfter(resolvePass, AZ::Name("ViewModeOverdrawCountPass")), true);

            AZ_Error(
                Window, backgroundPass && wireframeHiddenPass && wireframePass && countPass && resolvePass,
                "Could not create the view-mode passes for pipeline '%s'", renderPipeline.GetId().GetCStr());

            (backgroundPass && wireframeHiddenPass && wireframePass && countPass && resolvePass) &&
                (m_pipelinePasses[&renderPipeline] =
                     PipelinePasses{ backgroundPass, wireframeHiddenPass, wireframePass, countPass, resolvePass },
                 true);
        }

        void ViewModeFeatureProcessor::RefreshPipelinePasses(AZ::RPI::RenderPipeline& renderPipeline)
        {
            m_pipelinePasses.erase(&renderPipeline);

            auto* passSystem = AZ::RPI::PassSystemInterface::Get();
            auto backgroundFilter =
                AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ViewModeBackgroundPassTemplate"), &renderPipeline);
            auto wireframeHiddenFilter =
                AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ViewModeWireframeHiddenPassTemplate"), &renderPipeline);
            auto wireframeFilter =
                AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ViewModeWireframePassTemplate"), &renderPipeline);
            auto countFilter =
                AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ViewModeOverdrawCountPassTemplate"), &renderPipeline);
            auto resolveFilter =
                AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ViewModeOverdrawResolvePassTemplate"), &renderPipeline);

            AZ::RPI::Ptr<AZ::RPI::Pass> backgroundPass = passSystem->FindFirstPass(backgroundFilter);
            AZ::RPI::Ptr<AZ::RPI::Pass> wireframeHiddenPass = passSystem->FindFirstPass(wireframeHiddenFilter);
            AZ::RPI::Ptr<AZ::RPI::Pass> wireframePass = passSystem->FindFirstPass(wireframeFilter);
            AZ::RPI::Ptr<AZ::RPI::Pass> countPass = passSystem->FindFirstPass(countFilter);
            AZ::RPI::Ptr<AZ::RPI::Pass> resolvePass = passSystem->FindFirstPass(resolveFilter);

            (backgroundPass && wireframeHiddenPass && wireframePass && countPass && resolvePass) &&
                (m_pipelinePasses[&renderPipeline] =
                     PipelinePasses{ backgroundPass, wireframeHiddenPass, wireframePass, countPass, resolvePass },
                 true);
        }

        void ViewModeFeatureProcessor::OnRenderPipelineChanged(
            AZ::RPI::RenderPipeline* renderPipeline, AZ::RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            AZ_Assert(renderPipeline, "OnRenderPipelineChanged called with a null render pipeline");

            const bool removed = changeType == AZ::RPI::SceneNotification::RenderPipelineChangeType::Removed;
            removed && (m_pipelinePasses.erase(renderPipeline), true);
            !removed && (RefreshPipelinePasses(*renderPipeline), true);
        }

        bool ViewModeFeatureProcessor::AnyViewModePassEnabled() const
        {
            bool anyEnabled = false;
            for (const auto& [pipeline, passes] : m_pipelinePasses)
            {
                anyEnabled = anyEnabled || (passes.m_wireframe && passes.m_wireframe->IsEnabled()) ||
                    (passes.m_count && passes.m_count->IsEnabled());
            }
            return anyEnabled;
        }

        void ViewModeFeatureProcessor::OnBeginPrepareRender()
        {
            const bool active = AnyViewModePassEnabled();
            (!active && !m_meshes.empty()) && (m_meshes.clear(), m_meshes.shrink_to_fit(), true);
            (active && EnsureAssets()) && (RefreshTrackedMeshes(), RefreshTransforms(), true);
        }

        void ViewModeFeatureProcessor::RefreshTrackedMeshes()
        {
            auto* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<AZ::Render::MeshFeatureProcessorInterface>();

            AZStd::vector<AZStd::pair<AZ::EntityId, AZ::Data::Instance<AZ::RPI::Model>>> current;
            AZ::ComponentApplicationBus::Broadcast(
                &AZ::ComponentApplicationRequests::EnumerateEntities,
                [this, &current, meshFeatureProcessor](AZ::Entity* entity)
                {
                    // View modes render only the entities living in this scene's world.
                    if (AZ::RPI::Scene::GetSceneForEntityId(entity->GetId()) != GetParentScene())
                    {
                        return;
                    }

                    AZ::Data::Instance<AZ::RPI::Model> model;
                    AZ::Render::MeshComponentRequestBus::EventResult(
                        model, entity->GetId(), &AZ::Render::MeshComponentRequests::GetModel);

                    const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* meshHandle = nullptr;
                    (!model && meshFeatureProcessor) &&
                        (AZ::Render::MeshHandleStateRequestBus::EventResult(
                             meshHandle, entity->GetId(), &AZ::Render::MeshHandleStateRequests::GetMeshHandle),
                         true);
                    (!model && meshHandle && meshHandle->IsValid()) &&
                        (model = meshFeatureProcessor->GetModel(*meshHandle), true);

                    model && (current.emplace_back(entity->GetId(), model), true);
                });

            bool changed = current.size() != m_meshes.size();
            for (size_t i = 0; i < current.size() && !changed; ++i)
            {
                changed = current[i].first != m_meshes[i].m_entityId || current[i].second != m_meshes[i].m_model;
            }

            changed && (RebuildTrackedMeshes(current), true);

            for (TrackedMesh& mesh : m_meshes)
            {
                for (AZ::RPI::MeshDrawPacket& drawPacket : mesh.m_drawPackets)
                {
                    drawPacket.Update(*GetParentScene(), false);
                }
            }
        }

        void ViewModeFeatureProcessor::RebuildTrackedMeshes(
            const AZStd::vector<AZStd::pair<AZ::EntityId, AZ::Data::Instance<AZ::RPI::Model>>>& current)
        {
            AZ_Assert(m_material, "RebuildTrackedMeshes called before the view-mode material was loaded");
            AZ_Assert(m_srgShaderAsset.IsReady(), "RebuildTrackedMeshes called before the object SRG shader was loaded");

            m_meshes.clear();
            m_meshes.reserve(current.size());

            for (const auto& [entityId, model] : current)
            {
                TrackedMesh mesh;
                mesh.m_entityId = entityId;
                mesh.m_model = model;
                mesh.m_objectSrg = AZ::RPI::ShaderResourceGroup::Create(m_srgShaderAsset, AZ::Name("ViewModeObjectSrg"));
                AZ_Error(
                    Window, mesh.m_objectSrg, "Failed to create a ViewModeObjectSrg for entity %s",
                    entityId.ToString().c_str());

                const AZ::u64 entityHash = AZStd::hash<AZ::u64>{}(static_cast<AZ::u64>(entityId));
                const float hue = static_cast<float>(entityHash % 3600u) / 3600.0f;
                const float red = AZ::GetClamp(fabsf(hue * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f);
                const float green = AZ::GetClamp(2.0f - fabsf(hue * 6.0f - 2.0f), 0.0f, 1.0f);
                const float blue = AZ::GetClamp(2.0f - fabsf(hue * 6.0f - 4.0f), 0.0f, 1.0f);
                const AZ::Vector4 wireColor(
                    red * 0.75f + 0.25f, green * 0.75f + 0.25f, blue * 0.75f + 0.25f, 0.85f);
                mesh.m_objectSrg && (mesh.m_objectSrg->SetConstant(mesh.m_colorIndex, wireColor), true);

                const auto lods = model->GetLods();
                const size_t meshCount = (mesh.m_objectSrg && !lods.empty()) ? lods[0]->GetMeshes().size() : 0;
                for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
                {
                    AZ::RPI::MeshDrawPacket drawPacket(*lods[0], meshIndex, -1, m_material, mesh.m_objectSrg);
                    drawPacket.Update(*GetParentScene(), true);
                    mesh.m_drawPackets.push_back(AZStd::move(drawPacket));
                }

                mesh.m_objectSrg && (m_meshes.push_back(AZStd::move(mesh)), true);
            }
        }

        void ViewModeFeatureProcessor::RefreshTransforms()
        {
            for (TrackedMesh& mesh : m_meshes)
            {
                AZ::Transform world = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(world, mesh.m_entityId, &AZ::TransformBus::Events::GetWorldTM);

                const bool changed = !world.IsClose(mesh.m_transform);
                changed &&
                    (mesh.m_transform = world,
                     mesh.m_objectSrg->SetConstant(mesh.m_worldIndex, AZ::Matrix4x4::CreateFromTransform(world)),
                     mesh.m_objectSrg->Compile(), true);
            }
        }

        void ViewModeFeatureProcessor::Render(const RenderPacket& packet)
        {
            const bool active = AnyViewModePassEnabled() && !m_meshes.empty();
            active &&
                ([this, &packet]
                 {
                     for (const AZ::RPI::ViewPtr& view : packet.m_views)
                     {
                         for (const TrackedMesh& mesh : m_meshes)
                         {
                             for (const AZ::RPI::MeshDrawPacket& drawPacket : mesh.m_drawPackets)
                             {
                                 const AZ::RHI::DrawPacket* rhiDrawPacket = drawPacket.GetRHIDrawPacket();
                                 rhiDrawPacket && (view->AddDrawPacket(rhiDrawPacket), true);
                             }
                         }
                     }
                 }(),
                 true);
        }
    } // namespace Render
} // namespace AZ
