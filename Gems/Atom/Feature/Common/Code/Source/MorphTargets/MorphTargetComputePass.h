/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshFeatureProcessor;

        //! The morph target compute pass submits dispatch items for morph targets. The dispatch items are cleared every frame, so it needs to be re-populated.
        class MorphTargetComputePass
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(MorphTargetComputePass);
        public:
            AZ_RTTI(AZ::Render::MorphTargetComputePass, "{14EEACDF-C1BB-4BFC-BB27-6821FDE276B0}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(MorphTargetComputePass, SystemAllocator);

            MorphTargetComputePass(const RPI::PassDescriptor& descriptor);

            static RPI::Ptr<MorphTargetComputePass> Create(const RPI::PassDescriptor& descriptor);

            Data::Instance<RPI::Shader> GetShader() const;

            void SetFeatureProcessor(SkinnedMeshFeatureProcessor* m_skinnedMeshFeatureProcessor);
        private:
            void BuildInternal() override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            SkinnedMeshFeatureProcessor* m_skinnedMeshFeatureProcessor = nullptr;
        };
    }
}
