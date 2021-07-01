/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/Bloom/BloomSettings.h>
#include <PostProcessing/BloomParentPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<BloomParentPass> BloomParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<BloomParentPass> pass = aznew BloomParentPass(descriptor);
            return pass;
        }

        BloomParentPass::BloomParentPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        { }

        bool BloomParentPass::IsEnabled() const
        {
            if (!ParentPass::IsEnabled())
            {
                return false;
            }
            const RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            const RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
            if (!fp)
            {
                return false;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            const BloomSettings* bloomSettings = postProcessSettings->GetBloomSettings();
            return (bloomSettings != nullptr) && bloomSettings->GetEnabled();
        }

    }   // namespace Render
}   // namespace AZ
