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

// include the required headers
#include "ThreadData.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ThreadData, ThreadDataAllocator, 0)

    // default constructor
    ThreadData::ThreadData()
        : BaseObject()
    {
        mThreadIndex    = MCORE_INVALIDINDEX32;
    }


    // constructor
    ThreadData::ThreadData(uint32 threadIndex)
    {
        mThreadIndex    = threadIndex;
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
        mThreadIndex = index;
    }


    uint32 ThreadData::GetThreadIndex() const
    {
        return mThreadIndex;
    }
}   // namespace EMotionFX
