/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Silhouette/SilhouetteCompositePass.h>
#include <Silhouette/SilhouetteFeatureProcessor.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render
{
    SilhouetteCompositePass::SilhouetteCompositePass(const RPI::PassDescriptor& descriptor)
        : RPI::FullscreenTrianglePass(descriptor)
    {
    }

    void SilhouetteCompositePass::InitializeInternal()
    {
        RPI::FullscreenTrianglePass::InitializeInternal();
        auto* scene = GetScene();
        if (!scene)
        {
            return;
        }

        m_featureProcessor = scene->GetFeatureProcessor<SilhouetteFeatureProcessor>();
    }

    RPI::Ptr<SilhouetteCompositePass> SilhouetteCompositePass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<SilhouetteCompositePass> pass = aznew SilhouetteCompositePass(descriptor);
        return pass;
    }

    void SilhouetteCompositePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        RPI::FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);
        if (m_featureProcessor)
        {
            // We check if we need to do the composite pass
            // We don't enable/disable the pass because that causes rebuilding the renderpass when subpasses are being merged.
            frameGraph.SetEstimatedItemCount(m_featureProcessor->NeedsCompositePass() ? 1 : 0);
        }
    }

    void SilhouetteCompositePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
    {
        const auto* scope = static_cast<const SilhouetteCompositePass*>(this)->GetScope();
        if (scope->GetEstimatedItemCount())
        {
            RPI::FullscreenTrianglePass::BuildCommandListInternal(context);
        }
    }

} // namespace AZ::Render

