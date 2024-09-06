/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <webgpu/webgpu_cpp.h>
#include <RHI/Conversions.h>
#include <AzCore/std/string/string.h>

namespace AZ::WebGPU
{
    inline void AssertSuccess([[maybe_unused]] wgpu::Status result)
    {
        if (result != wgpu::Status::Success)
        {
            AZ_Assert(false, "ASSERT: WebGPU API method failed: %s", ToString(result));
        }
    }
}
