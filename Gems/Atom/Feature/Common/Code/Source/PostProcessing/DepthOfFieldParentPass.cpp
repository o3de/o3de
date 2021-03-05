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
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
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
