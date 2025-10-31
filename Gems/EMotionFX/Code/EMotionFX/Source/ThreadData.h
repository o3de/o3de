/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include "AnimGraphPosePool.h"
#include "AnimGraphRefCountedDataPool.h"
#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{
    /**
     *
     *
     */
    class EMFX_API ThreadData
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static ThreadData* Create();
        static ThreadData* Create(uint32 threadIndex);

        void SetThreadIndex(uint32 index);
        uint32 GetThreadIndex() const;

        MCORE_INLINE const AnimGraphPosePool& GetPosePool() const                          { return m_posePool; }
        MCORE_INLINE AnimGraphPosePool& GetPosePool()                                      { return m_posePool; }

        MCORE_INLINE AnimGraphRefCountedDataPool& GetRefCountedDataPool()                  { return m_refCountedDataPool; }
        MCORE_INLINE const AnimGraphRefCountedDataPool& GetRefCountedDataPool() const      { return m_refCountedDataPool; }

    private:
        uint32                          m_threadIndex;
        AnimGraphPosePool              m_posePool;
        AnimGraphRefCountedDataPool    m_refCountedDataPool;

        ThreadData();
        ThreadData(uint32 threadIndex);
        ~ThreadData();
    };
}   // namespace EMotionFX
