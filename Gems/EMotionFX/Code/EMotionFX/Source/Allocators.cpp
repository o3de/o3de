/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    void Allocators::ShrinkPools()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GarbageCollect();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Get().GarbageCollect();
    }

} // EMotionFX namespace
