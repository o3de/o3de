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