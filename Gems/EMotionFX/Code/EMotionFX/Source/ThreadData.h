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

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include "AnimGraphPosePool.h"
#include "AnimGraphRefCountedDataPool.h"
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    /**
     *
     *
     */
    class EMFX_API ThreadData
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static ThreadData* Create();
        static ThreadData* Create(uint32 threadIndex);

        void SetThreadIndex(uint32 index);
        uint32 GetThreadIndex() const;

        MCORE_INLINE const AnimGraphPosePool& GetPosePool() const                          { return mPosePool; }
        MCORE_INLINE AnimGraphPosePool& GetPosePool()                                      { return mPosePool; }

        MCORE_INLINE AnimGraphRefCountedDataPool& GetRefCountedDataPool()                  { return mRefCountedDataPool; }
        MCORE_INLINE const AnimGraphRefCountedDataPool& GetRefCountedDataPool() const      { return mRefCountedDataPool; }

    private:
        uint32                          mThreadIndex;
        AnimGraphPosePool              mPosePool;
        AnimGraphRefCountedDataPool    mRefCountedDataPool;

        ThreadData();
        ThreadData(uint32 threadIndex);
        ~ThreadData();
    };
}   // namespace EMotionFX
