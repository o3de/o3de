/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <./ShineFeatureProcessor.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>

namespace Shine
{
    void ShineFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShineFeatureProcessor, AZ::RPI::FeatureProcessor>()
                ->Version(0)
                ;
        }
    }

    void ShineFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline)
    {
        // Only add ShineParentPass if UIPass exists
        if (!renderPipeline->FindFirstPass(AZ::Name("UIPass")))
        {
            return;
        }

        // Get the pass request if it's not loaded
        if (!m_passRequestAsset)
        {
            const char* passRequestAssetFilePath = "Passes/ShinePassRequest.azasset";
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
            AZ_Error("Shine", false, "Failed to add Shine parent pass. Can't load PassRequest from %s", m_passRequestAsset.GetHint().c_str());
            return;
        }

        // Return if the pass to be created already exists
        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(passRequest->m_passName, renderPipeline);
        AZ::RPI::Pass* pass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (pass)
        {
            return;
        }

        // Create the pass
        AZ::RPI::Ptr<AZ::RPI::Pass> ShineParentPass  = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
        if (!ShineParentPass)
        {
            AZ_Error("Shine", false, "Create Shine parent pass from pass request failed");
            return;
        }

        // Insert the ShineParentPass before UIPass
        bool success = renderPipeline->AddPassBefore(ShineParentPass, AZ::Name("UIPass"));
        // only create pass resources if it was success
        if (!success)
        {
            AZ_Error("Shine", false, "Add the Shine parent pass to render pipeline [%s] failed",
                renderPipeline->GetId().GetCStr());
        }
    }

}
