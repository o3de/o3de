/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomCore/std/containers/vector_set.h>

#include <Atom/RPI.Public/Pass/PipelinePass.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/RPI.Reflect/Pass/PipelinePassData.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<PipelinePass> PipelinePass::Create(const PassDescriptor& descriptor)
        {
            Ptr<PipelinePass> pass = aznew PipelinePass(descriptor);
            return pass;
        }

        Ptr<ParentPass> PipelinePass::Recreate() const
        {
            PassDescriptor desc = GetPassDescriptor();
            Ptr<ParentPass> pass = aznew PipelinePass(desc);
            return pass;
        }

        PipelinePass::PipelinePass(const PassDescriptor& descriptor)
            : ParentPass(descriptor)
        { }

        void PipelinePass::CreatePipelineAttachmentsFromPassData(const PipelinePassData& passData)
        {
            for (const PassImageAttachmentDesc& imageDesc : passData.m_imageAttachments)
            {
                m_pipeline->AddPipelineAttachment(CreateImageAttachment(imageDesc));
            }
            for (const PassBufferAttachmentDesc& bufferDesc : passData.m_bufferAttachments)
            {
                m_pipeline->AddPipelineAttachment(CreateBufferAttachment(bufferDesc));
            }
        }

        void PipelinePass::BuildChildPasses()
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->Build();
            }
        }

        void PipelinePass::BuildChildPassesWithPipelineConnections(const PipelinePassData& passData)
        {
            for (const Ptr<Pass>& child : m_children)
            {
                child->Build();

                // Check if the pass is in the list of global pipeline connections
                // Do this between each pass build since a pass N may specify a global attachment that is referenced by pass N + 1
                for (const PipelineConnection& pipelineConnection : passData.m_pipelineConnections)
                {
                    if (pipelineConnection.m_childPass == child->GetName())
                    {
                        // Check if the pass has the specified binding
                        GlobalBinding bindingRef;
                        bindingRef.m_binding = child->FindAttachmentBinding(pipelineConnection.m_childPassBinding);

                        if (bindingRef.m_binding != nullptr)
                        {
                            // Add the binding to the pipeline for global reference
                            bindingRef.m_name = pipelineConnection.m_globalName;
                            bindingRef.m_pass = child.get();
                            m_pipeline->AddPipelineConnection(bindingRef);
                            child->m_flags.m_containsGlobalReference = true;
                        }
                    }
                }
            }
        }

        void PipelinePass::BuildInternal()
        {
            CreateChildPasses();

            const PipelinePassData* passData = azrtti_cast<PipelinePassData*>(m_passData.get());

            if (m_pipeline)
            {
                m_pipeline->ClearGlobalAttachmentsAndBindings();

                if (passData)
                {
                    CreatePipelineAttachmentsFromPassData(*passData);
                }

                AddPipelineAttachmentsAndConnectionsInternal();

                if (passData)
                {
                    BuildChildPassesWithPipelineConnections(*passData);
                }
                else
                {
                    BuildChildPasses();
                }
            }
            else
            {
                BuildChildPasses();
            }
        }

    }   // namespace RPI
}   // namespace AZ
