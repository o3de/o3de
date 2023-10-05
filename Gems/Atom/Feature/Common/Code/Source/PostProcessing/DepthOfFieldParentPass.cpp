/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/DepthOfField/DepthOfFieldSettings.h>
#include <PostProcessing/DepthOfFieldParentPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DepthOfFieldParentPass> DepthOfFieldParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthOfFieldParentPass> pass = aznew DepthOfFieldParentPass(descriptor);
            return pass;
        }

        DepthOfFieldParentPass::DepthOfFieldParentPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        { }

        bool DepthOfFieldParentPass::IsEnabled() const
        {
            if (!ParentPass::IsEnabled())
            {
                return false;
            }
            RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            if (!fp)
            {
                return false;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            DepthOfFieldSettings* dofSettings = postProcessSettings->GetDepthOfFieldSettings();
            return (dofSettings != nullptr) && dofSettings->GetEnabled();
        }

    }   // namespace Render
}   // namespace AZ
