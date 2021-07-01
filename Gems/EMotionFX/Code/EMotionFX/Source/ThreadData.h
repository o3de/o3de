/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
