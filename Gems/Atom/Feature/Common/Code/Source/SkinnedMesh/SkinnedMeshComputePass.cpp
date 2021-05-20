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
        
        void SkinnedMeshComputePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            if (m_skinnedMeshFeatureProcessor)
            {
                RHI::CommandList* commandList = context.GetCommandList();

                SetSrgsForDispatch(commandList);

                m_skinnedMeshFeatureProcessor->SubmitSkinningDispatchItems(commandList);
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

        void SkinnedMeshComputePass::OnShaderVariantReinitialized(const RPI::Shader& shader, const RPI::ShaderVariantId&, RPI::ShaderVariantStableId)
        {
            OnShaderReinitialized(shader);
        }
    }   // namespace Render
}   // namespace AZ
