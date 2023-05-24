/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SkinnedMesh/SkinnedMeshShaderOptionsCache.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshFeatureProcessor;

        //! The skinned mesh compute pass submits dispatch items for skinning. The dispatch items are cleared every frame, so it needs to be re-populated.
        class SkinnedMeshComputePass
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(SkinnedMeshComputePass);
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshComputePass, "{CE046FFC-B870-40EE-872A-DB0958B97CC3}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(SkinnedMeshComputePass, SystemAllocator);

            SkinnedMeshComputePass(const RPI::PassDescriptor& descriptor);

            static RPI::Ptr<SkinnedMeshComputePass> Create(const RPI::PassDescriptor& descriptor);

            Data::Instance<RPI::Shader> GetShader() const;

            void SetFeatureProcessor(SkinnedMeshFeatureProcessor* m_skinnedMeshFeatureProcessor);

        private:
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const RPI::Shader& shader) override;
            void OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant) override;

            SkinnedMeshFeatureProcessor* m_skinnedMeshFeatureProcessor = nullptr;
        };
    }
}
