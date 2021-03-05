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

// include required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/Array.h>



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

        void Resize(uint32 numPoses);

        AnimGraphPose* RequestPose(const ActorInstance* actorInstance);
        void FreePose(AnimGraphPose* pose);

        void FreeAllPoses();

        MCORE_INLINE uint32 GetNumFreePoses() const             { return mFreePoses.GetLength(); }
        MCORE_INLINE uint32 GetNumPoses() const                 { return mPoses.GetLength(); }
        MCORE_INLINE uint32 GetNumUsedPoses() const             { return (mPoses.GetLength() - mFreePoses.GetLength()); }
        MCORE_INLINE uint32 GetNumMaxUsedPoses() const          { return mMaxUsed; }
        MCORE_INLINE void ResetMaxUsedPoses()                   { mMaxUsed = 0; }

    private:
        MCore::Array<AnimGraphPose*>   mPoses;
        MCore::Array<AnimGraphPose*>   mFreePoses;
        uint32                         mMaxUsed;
    };
}   // namespace EMotionFX
