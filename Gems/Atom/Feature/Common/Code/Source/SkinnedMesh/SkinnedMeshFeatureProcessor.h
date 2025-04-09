/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SkinnedMesh/SkinnedMeshRenderProxy.h>
#include <SkinnedMesh/SkinnedMeshStatsCollector.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshFeatureProcessorInterface.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>

#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/Utils/StableDynamicArray.h>

#include <AzCore/base.h>

#include <AtomCore/std/parallel/concurrency_checker.h>

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshComputePass;
        class MorphTargetComputePass;
        class MeshFeatureProcessor;

        class SkinnedMeshFeatureProcessor final
            : public SkinnedMeshFeatureProcessorInterface
        {
            friend class SkinnedMeshStatsCollector;
        public:
            AZ_CLASS_ALLOCATOR(SkinnedMeshFeatureProcessor, AZ::SystemAllocator)

            AZ_RTTI(AZ::Render::SkinnedMeshFeatureProcessor, "{D1F44963-913F-4210-92E1-945FA306BED4}", AZ::Render::SkinnedMeshFeatureProcessorInterface);
            AZ_FEATURE_PROCESSOR(SkinnedMeshFeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            SkinnedMeshFeatureProcessor() = default;
            virtual ~SkinnedMeshFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;
            void OnRenderEnd() override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;
            void OnBeginPrepareRender() override;

            // SkinnedMeshFeatureProcessorInterface overrides ...
            SkinnedMeshHandle AcquireSkinnedMesh(const SkinnedMeshHandleDescriptor& desc) override;
            bool ReleaseSkinnedMesh(SkinnedMeshHandle& handle) override;
            void SetSkinningMatrices(const SkinnedMeshHandle& handle, const AZStd::vector<float>& data) override;
            void SetMorphTargetWeights(const SkinnedMeshHandle& handle, uint32_t lodIndex, const AZStd::vector<float>& weights) override;
            void EnableSkinning(const SkinnedMeshHandle& handle, uint32_t lodIndex, uint32_t meshIndex) override;
            void DisableSkinning(const SkinnedMeshHandle& handle, uint32_t lodIndex, uint32_t meshIndex) override;

            Data::Instance<RPI::Shader> GetSkinningShader() const;
            RPI::ShaderOptionGroup CreateSkinningShaderOptionGroup(const SkinnedMeshShaderOptions shaderOptions, SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedHandler);
            void OnSkinningShaderReinitialized(const Data::Instance<RPI::Shader> skinningShader);
            void SubmitSkinningDispatchItems(const RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex);
            void SetupSkinningScope(RHI::FrameGraphInterface frameGraph);

            Data::Instance<RPI::Shader> GetMorphTargetShader() const;
            void SubmitMorphTargetDispatchItems(const RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex);
            void SetupMorphTargetScope(RHI::FrameGraphInterface frameGraph);

        private:
            AZ_DISABLE_COPY_MOVE(SkinnedMeshFeatureProcessor);

            void InitSkinningAndMorphPass(RPI::RenderPipeline* renderPipeline);

            static const char* s_featureProcessorName;

            Data::Instance<RPI::Shader> m_skinningShader;
            CachedSkinnedMeshShaderOptions m_cachedSkinningShaderOptions;

            Data::Instance<RPI::Shader> m_morphTargetShader;

            AZStd::concurrency_checker m_renderProxiesChecker;
            StableDynamicArray<SkinnedMeshRenderProxy> m_renderProxies;
            AZStd::unique_ptr<SkinnedMeshStatsCollector> m_statsCollector;

            AZStd::unordered_set<const RHI::DispatchItem*> m_skinningDispatches;
            bool m_alreadyCreatedSkinningScopeThisFrame = false;

            AZStd::unordered_set<const RHI::DispatchItem*> m_morphTargetDispatches;
            bool m_alreadyCreatedMorphTargetScopeThisFrame = false;

            AZStd::mutex m_dispatchItemMutex;

        };
    } // namespace Render
} // namespace AZ
