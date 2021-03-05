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
