/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
