/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <DeferredMaterial/DeferredDrawPacketManager.h>
#include <DeferredMaterial/DeferredMeshDrawPacket.h>

#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <Atom/RPI.Public/Material/PersistentIndexAllocator.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

namespace AZ
{
    namespace Render
    {

        class MeshIterator;

        //! This feature processor manages Deferred DrawPackages for a Scene
        class DeferredMaterialFeatureProcessor : public RPI::FeatureProcessor
        {
            friend class MeshIterator;

        public:
            AZ_CLASS_ALLOCATOR(DeferredMaterialFeatureProcessor, AZ::SystemAllocator)

            using ModelId = AZ::Uuid;
            using MaterialTypeShaderId = AZStd::pair<int32_t, RPI::ShaderVariantId>;

            AZ_RTTI(AZ::Render::DeferredMaterialFeatureProcessor, "{9CA50AFC-206B-4F8A-80E8-2592CF1244B0}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            DeferredMaterialFeatureProcessor() = default;
            virtual ~DeferredMaterialFeatureProcessor() = default;

            //! Create a deferred draw-item for the referenced material types, if they don't exist yet
            void AddModel(const ModelId& uuid, ModelDataInstanceInterface* meshHandle, const Data::Instance<RPI::Model>& model);

            //! Removes a mesh and potentially the draw-item for the material type.
            void RemoveModel(const ModelId& uuid);

            //! A buffer with the DrawPacketId for the given deferred DrawListTag.
            //! This buffer contains one entry for every mesh in the scene, with the id of the deferred fullscreen-DrawItem that is
            //! responsible for that mesh. The buffer is kept in sync with the MeshInfoBuffer, and can be indexed using the meshInfoIndex.
            AZ::Data::Instance<AZ::RPI::Buffer> GetDrawPacketIdBuffer(const RHI::DrawListTag& drawListTag);

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Render(const RenderPacket&) override;
            void OnRenderPipelineChanged(
                RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

        private:
            AZ_DISABLE_COPY_MOVE(DeferredMaterialFeatureProcessor);

            DeferredDrawPacketManager m_drawPacketManager;

            struct MeshData
            {
                int32_t m_meshInfoIndex;
                DeferredMeshDrawPacket m_meshDrawPacket;
            };

            struct ModelLodData
            {
                AZStd::vector<MeshData> m_meshData;
            };

            struct ModelData
            {
                AZStd::vector<ModelLodData> m_lodData;
            };

            AZ::Data::Instance<AZ::RPI::Buffer>& GetOrCreateDrawPacketIdBuffer(
                const RHI::DrawListTag& drawListTag, const size_t numEntriesHint);
            void UpdateMeshDrawPackets(bool forceRebuild);
            void UpdateDrawPacketIdBuffers();
            void UpdateDrawSrgs();

            AZStd::unordered_map<ModelId, ModelData> m_modelData;

            struct FrameData
            {
                AZStd::unordered_map<RHI::DrawListTag, Data::Instance<RPI::Buffer>> m_drawPacketIdBuffers;
            };

            RHI::FrameCountMaxRingBuffer<FrameData> m_frameData;

            AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;

            bool m_needsUpdate = false;
            bool m_globalShaderOptionsChanged = false;
            AZStd::mutex m_updateMutex;
        };
    } // namespace Render
} // namespace AZ
