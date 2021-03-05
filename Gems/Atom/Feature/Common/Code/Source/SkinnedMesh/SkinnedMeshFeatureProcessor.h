/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
            void OnBeginPrepareRender() override;
            void OnEndPrepareRender() override;

            SkinnedMeshRenderProxyHandle AcquireRenderProxy(const SkinnedMeshRenderProxyDesc& desc);
            bool ReleaseRenderProxy(SkinnedMeshRenderProxyHandle& handle);

            RPI::Ptr<SkinnedMeshComputePass> GetSkinningPass() const;
            RPI::Ptr<MorphTargetComputePass> GetMorphTargetPass() const;
        private:
            AZ_DISABLE_COPY_MOVE(SkinnedMeshFeatureProcessor);

            void InitSkinningAndMorphPass();

            SkinnedMeshRenderProxyInterfaceHandle AcquireRenderProxyInterface(const SkinnedMeshRenderProxyDesc& desc) override;
            bool ReleaseRenderProxyInterface(SkinnedMeshRenderProxyInterfaceHandle& handle) override;

            static const char* s_featureProcessorName;
            RPI::Ptr<SkinnedMeshComputePass> m_skinningPass;
            RPI::Ptr<MorphTargetComputePass> m_morphTargetPass;
            AZStd::concurrency_checker m_renderProxiesChecker;
            StableDynamicArray<SkinnedMeshRenderProxy> m_renderProxies;
            AZStd::unique_ptr<SkinnedMeshStatsCollector> m_statsCollector;

            MeshFeatureProcessor* m_meshFeatureProcessor = nullptr;

        };
    } // namespace Render
} // namespace AZ
