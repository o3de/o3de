/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/ChromaticAbberationPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ChromaticAbberationPass> ChromaticAbberationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ChromaticAbberationPass> pass = aznew ChromaticAbberationPass(descriptor);
            return AZStd::move(pass);
        }

        ChromaticAbberationPass::ChromaticAbberationPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

    } // namespace Render
} // namespace AZ
