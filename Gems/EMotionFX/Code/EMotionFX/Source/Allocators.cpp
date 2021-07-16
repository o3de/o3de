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
    // Create all allocators
    void Allocators::Create()
    {
#define EMOTION_FX_ALLOCATOR_CREATE(ALLOCATOR_SEQUENCE) \
    AZ::AllocatorInstance<EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)>::Create();

        // Here we call the "Create" to create the allocator for all the items in the header's table (Step 4)
        AZ_SEQ_FOR_EACH(EMOTION_FX_ALLOCATOR_CREATE, EMOTIONFX_ALLOCATORS)

#undef EMOTION_FX_ALLOCATOR_CREATE

        // Create the pool allocators
        AZ::AllocatorInstance<AnimGraphConditionAllocator>::Create();
        AZ::AllocatorInstance<AnimGraphObjectDataAllocator>::Create();
        AZ::AllocatorInstance<AnimGraphObjectUniqueDataAllocator>::Create();
    }

    // Destroy all allocators
    void Allocators::Destroy()
    {
#define EMOTION_FX_ALLOCATOR_DESTROY(ALLOCATOR_SEQUENCE) \
    AZ::AllocatorInstance<EMOTIONFX_ALLOCATOR_SEQ_GET_NAME(ALLOCATOR_SEQUENCE)>::Destroy();

        // Here we call the "Destroy" to destroy the allocator for all the items in the header's table (Step 5)
        AZ_SEQ_FOR_EACH(EMOTION_FX_ALLOCATOR_DESTROY, EMOTIONFX_ALLOCATORS)

#undef EMOTION_FX_ALLOCATOR_DESTROY

        // Destroy the pool allocators
        AZ::AllocatorInstance<AnimGraphConditionAllocator>::Destroy();
        AZ::AllocatorInstance<AnimGraphObjectDataAllocator>::Destroy();
        AZ::AllocatorInstance<AnimGraphObjectUniqueDataAllocator>::Destroy();
    }

    void Allocators::ShrinkPools()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GarbageCollect();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Get().GarbageCollect();
    }

} // EMotionFX namespace
