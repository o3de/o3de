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
#include <AzCore/Math/Color.h>
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

            struct ViewModePassConnection
            {
                const char* m_slot = nullptr;
                const char* m_sourcePass = nullptr;
                const char* m_sourceSlot = nullptr;
            };

            struct ViewModePassDescriptor
            {
                const char* m_passName;
                const char* m_templateName;
                ViewModePassConnection m_connections[2];
            };

            //! Indexed by ViewModeFeatureProcessor::ViewModePass.
            constexpr ViewModePassDescriptor ViewModePassDescriptors[] = {
                { "ViewModeBackgroundPass",
                  "ViewModeBackgroundPassTemplate",
                  { { "ColorInputOutput", "PostProcessPass", "Output" } } },
                { "ViewModeWireframeHiddenPass",
                  "ViewModeWireframeHiddenPassTemplate",
                  { { "ColorInputOutput", "ViewModeBackgroundPass", "ColorInputOutput" },
                    { "DepthInputOutput", "DepthPrePass", "Depth" } } },
                { "ViewModeWireframePass",
                  "ViewModeWireframePassTemplate",
                  { { "ColorInputOutput", "ViewModeWireframeHiddenPass", "ColorInputOutput" },
                    { "DepthInputOutput", "DepthPrePass", "Depth" } } },
                { "ViewModeOverdrawCountPass",
                  "ViewModeOverdrawCountPassTemplate",
                  { { "DepthInputOutput", "DepthPrePass", "Depth" } } },
                { "ViewModeOverdrawResolvePass",
                  "ViewModeOverdrawResolvePassTemplate",
                  { { "Count", "ViewModeOverdrawCountPass", "Count" },
                    { "ColorInputOutput", "ViewModeWireframePass", "ColorInputOutput" } } },
            };

            //! Each pass is inserted after the one created before it; the first anchors on the pipeline.
            constexpr const char* ViewModePassPredecessor(size_t index)
            {
                return index == 0 ? "PostProcessPass" : ViewModePassDescriptors[index - 1].m_passName;
            }

            //! The templates are process-wide; whichever feature processor gets there first loads them.
            void LoadPassTemplates()
            {
                auto* passSystem = AZ::RPI::PassSystemInterface::Get();
                if (!g_viewModePassTemplatesLoaded && passSystem)
                {
                    g_viewModePassTemplatesLoaded = passSystem->LoadPassTemplateMappings(ViewModePassTemplatesAssetPath);
                }
            }
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

            // Render scenes are named after the framework scene that owns them (see Atom Bootstrap).
            auto* sceneSystem = AzFramework::SceneSystemInterface::Get();
            AZStd::shared_ptr<AzFramework::Scene> owningScene =
                sceneSystem ? sceneSystem->GetScene(GetParentScene()->GetName().GetStringView()) : nullptr;
            auto* renderScene = owningScene ? owningScene->FindSubsystemInScene<RPI::ScenePtr>() : nullptr;
            if (!renderScene || renderScene->get() != GetParentScene())
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
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ViewModeFeatureProcessor, AZ::RPI::FeatureProcessor>()->Version(0);
            }
        }

        void ViewModeFeatureProcessor::Activate()
        {
            LoadPassTemplates();
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

        template<typename AssetType>
        static void QueueAssetLoad(AZ::Data::Asset<AssetType>& asset, const char* productPath)
        {
            if (asset.GetId().IsValid())
            {
                return;
            }

            const AZ::Data::AssetId assetId =
                AZ::RPI::AssetUtils::GetAssetIdForProductPath(productPath, AZ::RPI::AssetUtils::TraceLevel::None);
            if (assetId.IsValid())
            {
                asset = AZ::Data::AssetManager::Instance().GetAsset<AssetType>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
            }
        }

        void ViewModeFeatureProcessor::QueueAssetLoads()
        {
            QueueAssetLoad(m_materialAsset, ViewModeMaterialProductPath);
            QueueAssetLoad(m_srgShaderAsset, ViewModeSrgShaderProductPath);
        }

        bool ViewModeFeatureProcessor::EnsureAssets()
        {
            QueueAssetLoads();

            if (!m_material && m_materialAsset.IsReady())
            {
                m_material = AZ::RPI::Material::FindOrCreate(m_materialAsset);
            }

            return m_material && m_srgShaderAsset.IsReady();
        }

        void ViewModeFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
        {
            AZ_Assert(renderPipeline, "AddRenderPasses called with a null render pipeline");

            LoadPassTemplates();

            if (renderPipeline->GetViewType() != AZ::RPI::ViewType::Default)
            {
                return;
            }

            auto existingFilter =
                AZ::RPI::PassFilter::CreateWithTemplateName(AZ::Name("ViewModeWireframePassTemplate"), renderPipeline);
            if (AZ::RPI::PassSystemInterface::Get()->FindFirstPass(existingFilter))
            {
                return;
            }

            const bool anchorsPresent = renderPipeline->FindFirstPass(AZ::Name("PostProcessPass")) != nullptr &&
                renderPipeline->FindFirstPass(AZ::Name("DepthPrePass")) != nullptr;
            AZ_Warning(
                Window, anchorsPresent,
                "Pipeline '%s' has no PostProcessPass/DepthPrePass anchors; the viewport view modes are unavailable in it",
                renderPipeline->GetId().GetCStr());

            if (anchorsPresent && g_viewModePassTemplatesLoaded)
            {
                InjectPasses(*renderPipeline);
            }
        }

        void ViewModeFeatureProcessor::InjectPasses(AZ::RPI::RenderPipeline& renderPipeline)
        {
            static_assert(AZ_ARRAY_SIZE(ViewModePassDescriptors) == ViewModePassCount, "Descriptors must match ViewModePass");

            auto* passSystem = AZ::RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "InjectPasses called without a pass system");

            PipelinePasses passes;
            for (size_t index = 0; index < ViewModePassCount; ++index)
            {
                const ViewModePassDescriptor& descriptor = ViewModePassDescriptors[index];

                AZ::RPI::PassRequest request;
                request.m_passName = AZ::Name(descriptor.m_passName);
                request.m_templateName = AZ::Name(descriptor.m_templateName);
                request.m_passEnabled = false;
                for (const ViewModePassConnection& connection : descriptor.m_connections)
                {
                    if (connection.m_slot)
                    {
                        request.AddInputConnection(AZ::RPI::PassConnection{
                            AZ::Name(connection.m_slot),
                            AZ::RPI::PassAttachmentRef{ AZ::Name(connection.m_sourcePass), AZ::Name(connection.m_sourceSlot) } });
                    }
                }

                passes[index] = passSystem->CreatePassFromRequest(&request);
                if (!passes[index])
                {
                    AZ_Error(
                        Window, false, "Could not create the view-mode passes for pipeline '%s'",
                        renderPipeline.GetId().GetCStr());
                    return;
                }

                renderPipeline.AddPassAfter(passes[index], AZ::Name(ViewModePassPredecessor(index)));
            }

            m_pipelinePasses[&renderPipeline] = passes;
        }

        void ViewModeFeatureProcessor::RefreshPipelinePasses(AZ::RPI::RenderPipeline& renderPipeline)
        {
            m_pipelinePasses.erase(&renderPipeline);

            auto* passSystem = AZ::RPI::PassSystemInterface::Get();
            PipelinePasses passes;
            for (size_t index = 0; index < ViewModePassCount; ++index)
            {
                auto filter = AZ::RPI::PassFilter::CreateWithTemplateName(
                    AZ::Name(ViewModePassDescriptors[index].m_templateName), &renderPipeline);
                passes[index] = passSystem->FindFirstPass(filter);
                if (!passes[index])
                {
                    return;
                }
            }

            m_pipelinePasses[&renderPipeline] = passes;
        }

        void ViewModeFeatureProcessor::OnRenderPipelineChanged(
            AZ::RPI::RenderPipeline* renderPipeline, AZ::RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            AZ_Assert(renderPipeline, "OnRenderPipelineChanged called with a null render pipeline");

            if (changeType == AZ::RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                m_pipelinePasses.erase(renderPipeline);
                return;
            }

            RefreshPipelinePasses(*renderPipeline);
        }

        bool ViewModeFeatureProcessor::AnyViewModePassEnabled() const
        {
            for (const auto& [pipeline, passes] : m_pipelinePasses)
            {
                if (passes[Wireframe]->IsEnabled() || passes[OverdrawCount]->IsEnabled())
                {
                    return true;
                }
            }
            return false;
        }

        void ViewModeFeatureProcessor::OnBeginPrepareRender()
        {
            if (!AnyViewModePassEnabled())
            {
                m_meshes.clear();
                m_meshes.shrink_to_fit();
                return;
            }

            if (EnsureAssets())
            {
                RefreshTrackedMeshes();
                RefreshTransforms();
            }
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

                    if (!model && meshFeatureProcessor)
                    {
                        const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* meshHandle = nullptr;
                        AZ::Render::MeshHandleStateRequestBus::EventResult(
                            meshHandle, entity->GetId(), &AZ::Render::MeshHandleStateRequests::GetMeshHandle);
                        if (meshHandle && meshHandle->IsValid())
                        {
                            model = meshFeatureProcessor->GetModel(*meshHandle);
                        }
                    }

                    if (model)
                    {
                        current.emplace_back(entity->GetId(), model);
                    }
                });

            bool changed = current.size() != m_meshes.size();
            for (size_t i = 0; i < current.size() && !changed; ++i)
            {
                changed = current[i].first != m_meshes[i].m_entityId || current[i].second != m_meshes[i].m_model;
            }

            if (changed)
            {
                RebuildTrackedMeshes(current);
            }

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
                if (!mesh.m_objectSrg)
                {
                    AZ_Error(Window, false, "Failed to create a ViewModeObjectSrg for entity %s", entityId.ToString().c_str());
                    continue;
                }

                // Give each entity a stable, well separated wireframe colour.
                const AZ::u64 entityHash = AZStd::hash<AZ::u64>{}(static_cast<AZ::u64>(entityId));
                const float hue = static_cast<float>(entityHash % 3600u) / 3600.0f;
                AZ::Color wireColor;
                wireColor.SetFromHSVRadians(hue * AZ::Constants::TwoPi, 0.75f, 1.0f);
                wireColor.SetA(0.85f);
                mesh.m_objectSrg->SetConstant(mesh.m_colorIndex, wireColor.GetAsVector4());

                const auto lods = model->GetLods();
                const size_t meshCount = lods.empty() ? 0 : lods[0]->GetMeshes().size();
                for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
                {
                    AZ::RPI::MeshDrawPacket drawPacket(*lods[0], meshIndex, -1, m_material, mesh.m_objectSrg);
                    drawPacket.Update(*GetParentScene(), true);
                    mesh.m_drawPackets.push_back(AZStd::move(drawPacket));
                }

                m_meshes.push_back(AZStd::move(mesh));
            }
        }

        void ViewModeFeatureProcessor::RefreshTransforms()
        {
            for (TrackedMesh& mesh : m_meshes)
            {
                AZ::Transform world = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(world, mesh.m_entityId, &AZ::TransformBus::Events::GetWorldTM);

                if (world.IsClose(mesh.m_transform))
                {
                    continue;
                }

                mesh.m_transform = world;
                mesh.m_objectSrg->SetConstant(mesh.m_worldIndex, AZ::Matrix4x4::CreateFromTransform(world));
                mesh.m_objectSrg->Compile();
            }
        }

        void ViewModeFeatureProcessor::Render(const RenderPacket& packet)
        {
            if (m_meshes.empty() || !AnyViewModePassEnabled())
            {
                return;
            }

            for (const AZ::RPI::ViewPtr& view : packet.m_views)
            {
                for (const TrackedMesh& mesh : m_meshes)
                {
                    for (const AZ::RPI::MeshDrawPacket& drawPacket : mesh.m_drawPackets)
                    {
                        if (const AZ::RHI::DrawPacket* rhiDrawPacket = drawPacket.GetRHIDrawPacket())
                        {
                            view->AddDrawPacket(rhiDrawPacket);
                        }
                    }
                }
            }
        }
    } // namespace Render
} // namespace AZ
