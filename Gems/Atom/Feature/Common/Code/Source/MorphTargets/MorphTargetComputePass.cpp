/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <MorphTargets/MorphTargetComputePass.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshOutputStreamManagerInterface.h>

#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/RHI/CommandList.h>

namespace AZ
{
    namespace Render
    {
        MorphTargetComputePass::MorphTargetComputePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        RPI::Ptr<MorphTargetComputePass> MorphTargetComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MorphTargetComputePass> pass = aznew MorphTargetComputePass(descriptor);
            return pass;
        }

        Data::Instance<RPI::Shader> MorphTargetComputePass::GetShader() const
        {
            return m_shader;
        }

        void MorphTargetComputePass::SetFeatureProcessor(SkinnedMeshFeatureProcessor* skinnedMeshFeatureProcessor)
        {
            m_skinnedMeshFeatureProcessor = skinnedMeshFeatureProcessor;
        }

        void MorphTargetComputePass::BuildInternal()
        {
            // The same buffer that skinning writes to is used to manage the computed vertex deltas that are passed from the
            // morph target pass to the skinning pass. This simplifies things by only requiring one class to manage the memory
            AttachBufferToSlot(Name{ "MorphTargetDeltaOutput" }, SkinnedMeshOutputStreamManagerInterface::Get()->GetBuffer());
        }

        void MorphTargetComputePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            if (m_skinnedMeshFeatureProcessor)
            {
                m_skinnedMeshFeatureProcessor->SetupMorphTargetScope(frameGraph);
            }

            ComputePass::SetupFrameGraphDependencies(frameGraph);
        }

        void MorphTargetComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            if (m_skinnedMeshFeatureProcessor)
            {
                SetSrgsForDispatch(context);

                m_skinnedMeshFeatureProcessor->SubmitMorphTargetDispatchItems(context, context.GetSubmitRange().m_startIndex, context.GetSubmitRange().m_endIndex);
            }
        }
    }   // namespace Render
}   // namespace AZ
