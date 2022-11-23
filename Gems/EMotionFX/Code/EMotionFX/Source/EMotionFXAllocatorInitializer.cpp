/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMotionFXAllocatorInitializer.h"
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    const char* const EMotionFXAllocatorInitializer::EMotionFXAllocatorInitializerTag = "EMotionFXAllocatorInitializer";

    EMotionFXAllocatorInitializer::EMotionFXAllocatorInitializer()
    {
        // Start EMotionFX allocator.
        AZ::AllocatorInstance<EMotionFX::Integration::EMotionFXAllocator>::Create();
    }

    EMotionFXAllocatorInitializer::~EMotionFXAllocatorInitializer()
    {
        // Destroy EMotionFX allocator.
        AZ::AllocatorInstance<EMotionFX::Integration::EMotionFXAllocator>::Destroy();
    }
}
