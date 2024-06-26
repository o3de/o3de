/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <SkinnedMesh/SkinnedMeshComputePass.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshOutputStreamManagerInterface.h>

#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/RHI/CommandList.h>

namespace AZ
{
    namespace Render
    {
        SkinnedMeshComputePass::SkinnedMeshComputePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        RPI::Ptr<SkinnedMeshComputePass> SkinnedMeshComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SkinnedMeshComputePass> pass = aznew SkinnedMeshComputePass(descriptor);
            return pass;
        }

        Data::Instance<RPI::Shader> SkinnedMeshComputePass::GetShader() const
        {
            return m_shader;
        }

        void SkinnedMeshComputePass::SetFeatureProcessor(SkinnedMeshFeatureProcessor* skinnedMeshFeatureProcessor)
        {
            m_skinnedMeshFeatureProcessor = skinnedMeshFeatureProcessor;
        }

        void SkinnedMeshComputePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            if (m_skinnedMeshFeatureProcessor)
            {
                m_skinnedMeshFeatureProcessor->SetupSkinningScope(frameGraph);
            }

            ComputePass::SetupFrameGraphDependencies(frameGraph);
        }

        void SkinnedMeshComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            if (m_skinnedMeshFeatureProcessor)
            {
                SetSrgsForDispatch(context);

                m_skinnedMeshFeatureProcessor->SubmitSkinningDispatchItems(context, context.GetSubmitRange().m_startIndex, context.GetSubmitRange().m_endIndex);
            }
        }

        void SkinnedMeshComputePass::OnShaderReinitialized(const RPI::Shader& shader)
        {
            ComputePass::OnShaderReinitialized(shader);
            if (m_skinnedMeshFeatureProcessor)
            {
                m_skinnedMeshFeatureProcessor->OnSkinningShaderReinitialized(m_shader);
            }
        }

        void SkinnedMeshComputePass::OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant)
        {
            ComputePass::OnShaderVariantReinitialized(shaderVariant);
            if (m_skinnedMeshFeatureProcessor)
            {
                m_skinnedMeshFeatureProcessor->OnSkinningShaderReinitialized(m_shader);
            }
        }
    }   // namespace Render
}   // namespace AZ
