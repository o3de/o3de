/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Allocators.h>


namespace AZ::Internal
{
    // Add implementation of PoolAllocatorHelper base class template RTTI functions
    // needed by the derived EMotionFX Pool allocators

    // Explicitly instantite the PoolAllocator schemas for the EMotionFX Allocators
    template class PoolAllocatorHelper<ThreadPoolSchemaHelper<EMotionFX::AnimGraphConditionAllocator>>;
    template class PoolAllocatorHelper<ThreadPoolSchemaHelper<EMotionFX::AnimGraphObjectDataAllocator>>;
    template class PoolAllocatorHelper<ThreadPoolSchemaHelper<EMotionFX::AnimGraphObjectUniqueDataAllocator>>;
}

namespace EMotionFX
{
    void Allocators::ShrinkPools()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GarbageCollect();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Get().GarbageCollect();
    }

} // EMotionFX namespace
