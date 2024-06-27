/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/MSAAResolvePass.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/DevicePipelineState.h>

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
            descriptor.m_attachmentId = copyDest.GetAttachment()->GetAttachmentId();
            descriptor.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::DontCare;
            descriptor.m_resolveAttachmentId = copySource.GetAttachment()->GetAttachmentId();
            frameGraph.UseResolveAttachment(descriptor);

            RenderPass::AddScopeQueryToFrameGraph(frameGraph);
        }


        void MSAAResolvePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
        }

        void MSAAResolvePass::BuildCommandListInternal([[maybe_unused]] const RHI::FrameGraphExecuteContext& context)
        {
        }

        bool MSAAResolvePass::IsEnabled() const
        {
            // check Pass base class first to see if the Pass is explicitly disabled
            if (!Pass::IsEnabled())
            {
                return false;
            }

            // check render pipeline MSAA sample count
            return (m_pipeline->GetRenderSettings().m_multisampleState.m_samples > 1);
        }
    }   // namespace RPI
}   // namespace AZ
