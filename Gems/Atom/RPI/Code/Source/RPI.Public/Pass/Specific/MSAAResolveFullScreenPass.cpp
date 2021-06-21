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

#include <Atom/RPI.Public/Pass/Specific/MSAAResolveFullScreenPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/RenderPipeline.h>

namespace AZ
{
    namespace RPI
    {
        RPI::Ptr<MSAAResolveFullScreenPass> MSAAResolveFullScreenPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MSAAResolveFullScreenPass> pass = aznew MSAAResolveFullScreenPass(descriptor);
            return AZStd::move(pass);
        }

        MSAAResolveFullScreenPass::MSAAResolveFullScreenPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        bool MSAAResolveFullScreenPass::IsEnabled() const
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
