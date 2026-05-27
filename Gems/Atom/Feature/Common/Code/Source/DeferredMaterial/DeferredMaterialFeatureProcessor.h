/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/DeferredMaterial/DeferredMaterialFeatureProcessorInterface.h>

#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <DeferredMaterial/DeferredDrawPacketManager.h>
#include <DeferredMaterial/DeferredMeshDrawPacket.h>

#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <Atom/RPI.Public/Material/PersistentIndexAllocator.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

namespace AZ::Render
{
    class MeshIterator;

    //! This feature processor manages Deferred DrawPackages for a Scene
    class DeferredMaterialFeatureProcessor : public DeferredMaterialFeatureProcessorInterface
    {
        friend class MeshIterator;

    public:
        AZ_CLASS_ALLOCATOR(DeferredMaterialFeatureProcessor, AZ::SystemAllocator)

        using ModelId = DeferredMaterialFeatureProcessorInterface::ModelId;
        using MaterialTypeShaderId = AZStd::pair<int32_t, RPI::ShaderVariantId>;

        AZ_RTTI(
            AZ::Render::DeferredMaterialFeatureProcessor,
            "{9CA50AFC-206B-4F8A-80E8-2592CF1244B0}",
            AZ::Render::DeferredMaterialFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        DeferredMaterialFeatureProcessor() = default;
        virtual ~DeferredMaterialFeatureProcessor() = default;

        //! Create a deferred draw-item for the referenced material types, if they don't exist yet
        void AddModel(const ModelId& uuid, ModelDataInstanceInterface* meshHandle, const Data::Instance<RPI::Model>& model) override;

        //! Removes a mesh and potentially the draw-item for the material type.
        void RemoveModel(const ModelId& uuid) override;

        //! A buffer with the DrawPacketId for the given deferred DrawListTag.
        //! This buffer contains one entry for every mesh in the scene, with the id of the deferred fullscreen-DrawItem that is
        //! responsible for that mesh. The buffer is kept in sync with the MeshInfoBuffer, and can be indexed using the meshInfoIndex.
        AZ::Data::Instance<AZ::RPI::Buffer> GetDrawPacketIdBuffer(const RHI::DrawListTag& drawListTag) override;

        //! Wheter Deferred rendering was enabled at startup, controlled by CVAR r_deferredRenderingEnabled
        bool IsEnabled() override
        {
            return m_enabled;
        }

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

        RPI::RingBuffer& GetOrCreateDrawPacketIdRingBuffer(const RHI::DrawListTag& drawListTag);
        void UpdateMeshDrawPackets(bool forceRebuild);
        void UpdateDrawPacketIdBuffers();
        void UpdateDrawSrgs();

        AZStd::unordered_map<ModelId, ModelData> m_modelData;

        AZStd::unordered_map<RHI::DrawListTag, RPI::RingBuffer> m_drawPacketIdBuffers;

        AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;

        bool m_needsUpdate = false;
        bool m_forceRebuild = false;
        bool m_enabled = false;
        AZStd::mutex m_updateMutex;
    };
} // namespace AZ::Render
