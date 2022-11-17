/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <./LyShineFeatureProcessor.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>

namespace LyShine
{
    void LyShineFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LyShineFeatureProcessor, AZ::RPI::FeatureProcessor>()
                ->Version(0)
                ;
        }
    }

    void LyShineFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
    {
        // Only add LyShineParentPass if UIPass exists
        if (!renderPipeline->FindFirstPass(AZ::Name("UIPass")))
        {
            return;
        }

        // Get the pass request if it's not loaded
        if (!m_passRequestAsset)
        {
            const char* passRequestAssetFilePath = "Passes/LyShinePassRequest.azasset";
            m_passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
                passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
            
        }

        const AZ::RPI::PassRequest *passRequest = nullptr;
        if (m_passRequestAsset->IsReady())
        {
            passRequest = m_passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
        }

        if (!passRequest)
        {
            AZ_Error("LyShine", false, "Failed to add LyShine parent pass. Can't load PassRequest from %s", m_passRequestAsset.GetHint().c_str());
            return;
        }

        AZ::RPI::PassRequest passRequestCopy = *passRequest;

        //TODO::This is not the best solution and require a better long term solution.
        //Check if DepthPrePass is part of the pipeline. This is possible for any pipelines built for tbdr gpus.
        if (!renderPipeline->FindFirstPass(AZ::Name("DepthPrePass")))
        {
            //Check if ForwardPass is available
            if (renderPipeline->FindFirstPass(AZ::Name("ForwardPass")))
            {
                // Find the depth attachment and hook that up via ForwardPass
                const AZ::Name depthAttachment = AZ::Name("DepthInputOutput");
                auto findIter = AZStd::find_if(passRequestCopy.m_connections.begin(), passRequestCopy.m_connections.end(), [depthAttachment](const AZ::RPI::PassConnection& entry)
                {
                    return entry.m_localSlot == depthAttachment;
                });

                if (findIter != passRequestCopy.m_connections.end())
                {
                    (*findIter).m_attachmentRef.m_pass = "ForwardPass";
                    (*findIter).m_attachmentRef.m_attachment = "DepthStencilOutput";
                }
            }
            passRequest = &passRequestCopy;
        }

        // Return if the pass to be created already exists
        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(passRequest->m_passName, renderPipeline);
        AZ::RPI::Pass* pass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (pass)
        {
            return;
        }

        // Create the pass
        AZ::RPI::Ptr<AZ::RPI::Pass> lyShineParentPass  = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
        if (!lyShineParentPass)
        {
            AZ_Error("LyShine", false, "Create LyShine parent pass from pass request failed");
            return;
        }

        // Insert the LyShineParentPass before UIPass
        bool success = renderPipeline->AddPassBefore(lyShineParentPass, AZ::Name("UIPass"));
        // only create pass resources if it was success
        if (!success)
        {
            AZ_Error("LyShine", false, "Add the LyShine parent pass to render pipeline [%s] failed",
                renderPipeline->GetId().GetCStr());
        }
    }

}
