/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DiffuseProbeGridDownsamplePass.h"
#include <DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DiffuseProbeGridDownsamplePass> DiffuseProbeGridDownsamplePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridDownsamplePass> pass = aznew DiffuseProbeGridDownsamplePass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseProbeGridDownsamplePass::DiffuseProbeGridDownsamplePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        bool DiffuseProbeGridDownsamplePass::IsEnabled() const
        {
            // only enabled if there are DiffuseProbeGrids present in the scene
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = m_pipeline->GetScene()->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            return (diffuseProbeGridFeatureProcessor && !diffuseProbeGridFeatureProcessor->GetProbeGrids().empty());
        }

    }   // namespace RPI
}   // namespace AZ
