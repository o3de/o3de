/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Draw/EditorStateMaskRenderer.h>
#include <Pass/EditorStatePassSystem.h>

#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Render
    {
        //! Feature processor for Editor Mode Feedback visual effect system.
        class EditorModeFeatureProcessor
            : public RPI::FeatureProcessor
            , private AZ::TickBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(EditorModeFeatureProcessor, SystemAllocator)
            AZ_RTTI(AZ::Render::EditorModeFeatureProcessor, "{78D40D57-F564-4ECD-B9F5-D8C9784B15D0}", AZ::RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void AddRenderPasses(RPI::RenderPipeline* renderPipeline) override;
            void Render(const RenderPacket& packet) override;
            void Simulate(const SimulatePacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, AZ::RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            // AZ::TickBus overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            //! Enable or disable the rendering editor mode feedback.
            void SetEnableRender(bool enableRender);

            //! Returns this scene's instance of the given editor state effect (null if not found).
            const EditorStateBase* GetEditorState(EditorState editorState) const;

        private:
            //! Returns the world this scene's effects follow. Resolved lazily: the owning
            //! framework scene links to the render scene only after feature processor activation.
            AzFramework::EntityContextId GetWorldId();

            AzFramework::EntityContextId m_worldId = AzFramework::EntityContextId::CreateNull();

            //! The pass system for the editor state feedback effects.
            AZStd::unique_ptr<EditorStatePassSystem> m_editorStatePassSystem;

            //! Map of all mask renderers for the draw tags used by the editor state feedback effects.
            AZStd::unordered_map<Name, EditorStateMaskRenderer> m_maskRenderers;

            //! Material for sending draw packets to the entity mask pass.
            Data::Instance<RPI::Material> m_maskMaterial = nullptr;
        };

        //! Feature processor for the wireframe and quad-overdraw viewport view modes.
        class ViewModeFeatureProcessor final
            : public RPI::FeatureProcessor
        {
        public:
            AZ_CLASS_ALLOCATOR(ViewModeFeatureProcessor, SystemAllocator)
            AZ_RTTI(AZ::Render::ViewModeFeatureProcessor, "{E79D0D99-BAFF-41C8-A8FA-6823C09F92A0}", AZ::RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void AddRenderPasses(RPI::RenderPipeline* renderPipeline) override;
            void Render(const RenderPacket& packet) override;
            void OnBeginPrepareRender() override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineChanged(
                RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

        private:
            struct TrackedMesh
            {
                EntityId m_entityId;
                Data::Instance<RPI::Model> m_model;
                Data::Instance<RPI::ShaderResourceGroup> m_objectSrg;
                RHI::ShaderInputNameIndex m_worldIndex = "m_world";
                RHI::ShaderInputNameIndex m_colorIndex = "m_color";
                Transform m_transform = Transform::CreateUniformScale(0.0f);
                AZStd::vector<RPI::MeshDrawPacket> m_drawPackets;
            };

            struct PipelinePasses
            {
                RPI::Ptr<RPI::Pass> m_background;
                RPI::Ptr<RPI::Pass> m_wireframeHidden;
                RPI::Ptr<RPI::Pass> m_wireframe;
                RPI::Ptr<RPI::Pass> m_count;
                RPI::Ptr<RPI::Pass> m_resolve;
            };

            bool AnyViewModePassEnabled() const;
            void QueueAssetLoads();
            bool EnsureAssets();
            void InjectPasses(RPI::RenderPipeline& renderPipeline);
            void RefreshPipelinePasses(RPI::RenderPipeline& renderPipeline);
            void RefreshTrackedMeshes();
            void RebuildTrackedMeshes(const AZStd::vector<AZStd::pair<EntityId, Data::Instance<RPI::Model>>>& current);
            void RefreshTransforms();

            Data::Instance<RPI::Material> m_material;
            Data::Asset<RPI::MaterialAsset> m_materialAsset;
            Data::Asset<RPI::ShaderAsset> m_srgShaderAsset;
            AZStd::vector<TrackedMesh> m_meshes;
            AZStd::unordered_map<RPI::RenderPipeline*, PipelinePasses> m_pipelinePasses;
        };
    } // namespace Render
} // namespace AZ
