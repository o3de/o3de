/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DiffuseGlobalIllumination/DiffuseProbeGridDownsamplePass.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        // --- Dedicated class for disabling ---
        // Since this class was created to facilitate disabling of the associated pass, it could be removed in future if an alternative method is found

        RPI::Ptr<DiffuseProbeGridDownsamplePass> DiffuseProbeGridDownsamplePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseProbeGridDownsamplePass> pass = aznew DiffuseProbeGridDownsamplePass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseProbeGridDownsamplePass::DiffuseProbeGridDownsamplePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        { }

        bool DiffuseProbeGridDownsamplePass::IsEnabled() const
        {
            const RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor =
                scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().empty())
            {
                // no diffuse probe grids
                return false;
            }

            return true;
        }
    } // namespace Render
} // namespace AZ
