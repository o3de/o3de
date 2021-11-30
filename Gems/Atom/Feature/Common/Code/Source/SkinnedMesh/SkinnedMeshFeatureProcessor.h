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
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
            void OnBeginPrepareRender() override;

            SkinnedMeshRenderProxyHandle AcquireRenderProxy(const SkinnedMeshRenderProxyDesc& desc);
            bool ReleaseRenderProxy(SkinnedMeshRenderProxyHandle& handle);

            Data::Instance<RPI::Shader> GetSkinningShader() const;
            RPI::ShaderOptionGroup CreateSkinningShaderOptionGroup(const SkinnedMeshShaderOptions shaderOptions, SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedHandler);
            void OnSkinningShaderReinitialized(const Data::Instance<RPI::Shader> skinningShader);
            void SubmitSkinningDispatchItems(RHI::CommandList* commandList);

            Data::Instance<RPI::Shader> GetMorphTargetShader() const;
            void SubmitMorphTargetDispatchItems(RHI::CommandList* commandList);
        private:
            AZ_DISABLE_COPY_MOVE(SkinnedMeshFeatureProcessor);

            void InitSkinningAndMorphPass(RPI::RenderPipeline* renderPipeline);

            SkinnedMeshRenderProxyInterfaceHandle AcquireRenderProxyInterface(const SkinnedMeshRenderProxyDesc& desc) override;
            bool ReleaseRenderProxyInterface(SkinnedMeshRenderProxyInterfaceHandle& handle) override;

            static const char* s_featureProcessorName;

            Data::Instance<RPI::Shader> m_skinningShader;
            CachedSkinnedMeshShaderOptions m_cachedSkinningShaderOptions;

            Data::Instance<RPI::Shader> m_morphTargetShader;

            AZStd::concurrency_checker m_renderProxiesChecker;
            StableDynamicArray<SkinnedMeshRenderProxy> m_renderProxies;
            AZStd::unique_ptr<SkinnedMeshStatsCollector> m_statsCollector;

            MeshFeatureProcessor* m_meshFeatureProcessor = nullptr;
            AZStd::unordered_set<const RHI::DispatchItem*> m_skinningDispatches;
            AZStd::unordered_set<const RHI::DispatchItem*> m_morphTargetDispatches;
            AZStd::mutex m_dispatchItemMutex;

        };
    } // namespace Render
} // namespace AZ
