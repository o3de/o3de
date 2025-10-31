/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "ThreadData.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ThreadData, ThreadDataAllocator)

    // default constructor
    ThreadData::ThreadData()
        : MCore::RefCounted()
    {
        m_threadIndex    = MCORE_INVALIDINDEX32;
    }


    // constructor
    ThreadData::ThreadData(uint32 threadIndex)
    {
        m_threadIndex    = threadIndex;
    }


    // destructor
    ThreadData::~ThreadData()
    {
    }


    // create
    ThreadData* ThreadData::Create()
    {
        return aznew ThreadData();
    }


    // create
    ThreadData* ThreadData::Create(uint32 threadIndex)
    {
        return aznew ThreadData(threadIndex);
    }


    void ThreadData::SetThreadIndex(uint32 index)
    {
        m_threadIndex = index;
    }


    uint32 ThreadData::GetThreadIndex() const
    {
        return m_threadIndex;
    }
}   // namespace EMotionFX
