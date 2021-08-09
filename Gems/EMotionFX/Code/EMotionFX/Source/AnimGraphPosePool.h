/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>



namespace EMotionFX
{
    // forward declarations
    class AnimGraphPose;
    class ActorInstance;


    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphPosePool
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphPosePool, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL);

    public:
        AnimGraphPosePool();
        ~AnimGraphPosePool();

        void Resize(size_t numPoses);

        AnimGraphPose* RequestPose(const ActorInstance* actorInstance);
        void FreePose(AnimGraphPose* pose);

        void FreeAllPoses();

        MCORE_INLINE size_t GetNumFreePoses() const             { return mFreePoses.size(); }
        MCORE_INLINE size_t GetNumPoses() const                 { return mPoses.size(); }
        MCORE_INLINE size_t GetNumUsedPoses() const             { return mPoses.size() - mFreePoses.size(); }
        MCORE_INLINE size_t GetNumMaxUsedPoses() const          { return mMaxUsed; }
        MCORE_INLINE void ResetMaxUsedPoses()                   { mMaxUsed = 0; }

    private:
        AZStd::vector<AnimGraphPose*>   mPoses;
        AZStd::vector<AnimGraphPose*>   mFreePoses;
        size_t                          mMaxUsed;
    };
}   // namespace EMotionFX
