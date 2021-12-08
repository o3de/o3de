/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DiffuseGlobalIllumination/DiffuseCompositePass.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        // --- Dedicated class for disabling ---

        RPI::Ptr<DiffuseCompositePass> DiffuseCompositePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DiffuseCompositePass> pass = aznew DiffuseCompositePass(descriptor);
            return AZStd::move(pass);
        }

        DiffuseCompositePass::DiffuseCompositePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        { }

        bool DiffuseCompositePass::IsEnabled() const
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
