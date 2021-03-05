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

#include "EMotionFXAllocatorInitializer.h"
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    const char* const EMotionFXAllocatorInitializer::EMotionFXAllocatorInitializerTag = "EMotionFXAllocatorInitializer";

    EMotionFXAllocatorInitializer::EMotionFXAllocatorInitializer()
    {
        // Start EMotionFX allocator.
        EMotionFX::Integration::EMotionFXAllocator::Descriptor allocatorDescriptor;
        AZ::AllocatorInstance<EMotionFX::Integration::EMotionFXAllocator>::Create(allocatorDescriptor);
    }

    EMotionFXAllocatorInitializer::~EMotionFXAllocatorInitializer()
    {
        // Destroy EMotionFX allocator.
        AZ::AllocatorInstance<EMotionFX::Integration::EMotionFXAllocator>::Destroy();
    }
}