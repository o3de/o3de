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

#include <Atom/RPI.Public/Pass/MSAAResolvePass.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/PipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<MSAAResolvePass> MSAAResolvePass::Create(const PassDescriptor& descriptor)
        {
            Ptr<MSAAResolvePass> pass = aznew MSAAResolvePass(descriptor);
            return pass;
        }

        MSAAResolvePass::MSAAResolvePass(const PassDescriptor& descriptor)
            : RenderPass(descriptor)
        {
        }

        void MSAAResolvePass::BuildInternal()
        {
            AZ_Assert(GetOutputCount() != 0, "MSAAResolvePass %s has no outputs to render to.", GetPathName().GetCStr());
        }

        void MSAAResolvePass::FrameBeginInternal(FramePrepareParams params)
        {
            RenderPass::FrameBeginInternal(params);
        }

        void MSAAResolvePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            // Manually declare attachments since the resolve attachment is not supported by RenderPass
            AZ_Assert(GetInputCount() == 1, "MSAAResolvePass only supports a single Input");
            AZ_Assert(GetOutputCount() == 1, "MSAAResolvePass only supports a single Output");

            const AZ::RPI::PassAttachmentBinding& copySource = GetInputBinding(0);
            const AZ::RPI::PassAttachmentBinding& copyDest = GetOutputBinding(0);

            frameGraph.UseColorAttachment(copySource.m_unifiedScopeDesc.GetAsImage());

            RHI::ResolveScopeAttachmentDescriptor descriptor;
            descriptor.m_attachmentId = copyDest.m_attachment->GetAttachmentId();
            descriptor.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::DontCare;
            descriptor.m_resolveAttachmentId = copySource.m_attachment->GetAttachmentId();
            frameGraph.UseResolveAttachment(descriptor);

            RenderPass::AddScopeQueryToFrameGraph(frameGraph);
        }


        void MSAAResolvePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
        }

        void MSAAResolvePass::BuildCommandListInternal([[maybe_unused]] const RHI::FrameGraphExecuteContext& context)
        {
        }
    }   // namespace RPI
}   // namespace AZ
