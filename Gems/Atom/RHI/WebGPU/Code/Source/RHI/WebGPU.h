/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformDef.h>
#include <webgpu/webgpu_cpp.h>
#include <RHI/Conversions.h>
#include <Atom/RHI/ObjectCollector.h>
#include <AzCore/std/string/string.h>

namespace AZ::WebGPU
{
    //! Mutex used for synchronization operations. It's null until multithreading is supported on WebGPU.
    using mutex = RHI::NullMutex;

    //! Min aligment for the size when doing a mapping operation of a buffer
    constexpr uint32_t MapSizeAligment = 4;
    //! Min aligment for the offset when doing a mapping operation of a buffer
    constexpr uint32_t MapOffsetAligment = 8;

    AZ_FORCE_INLINE void AssertSuccess([[maybe_unused]] wgpu::Status result)
    {
        AZ_Assert(result == wgpu::Status::Success, "ASSERT: WebGPU API method failed: %s", ToString(result));
    }

#define RETURN_RESULT_IF_UNSUCCESSFUL(result)                                                                                              \
    if (result != RHI::ResultCode::Success)                                                                                                \
    {                                                                                                                                      \
        return result;                                                                                                                     \
    }
}
