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
#include "Pose.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;

    /**
     * A pose of the character.
     * This includes transformation data.
     */
    class EMFX_API AnimGraphPose
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphPose, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_POSE);

    public:
        enum
        {
            FLAG_INUSE      = 1 << 0
        };

        AnimGraphPose();
        AnimGraphPose(const AnimGraphPose& other);
        ~AnimGraphPose();

        void LinkToActorInstance(const ActorInstance* actorInstance);
        void InitFromBindPose(const ActorInstance* actorInstance);

        MCORE_INLINE uint32 GetNumNodes() const                 { return mPose.GetNumTransforms(); }
        MCORE_INLINE const Pose& GetPose() const                { return mPose; }
        MCORE_INLINE Pose& GetPose()                            { return mPose; }
        MCORE_INLINE void SetPose(const Pose& pose)             { mPose = pose; }
        MCORE_INLINE const ActorInstance* GetActorInstance() const    { return mPose.GetActorInstance(); }

        MCORE_INLINE bool GetIsInUse() const                    { return (mFlags & FLAG_INUSE); }
        MCORE_INLINE void SetIsInUse(bool inUse)
        {
            if (inUse)
            {
                mFlags |= FLAG_INUSE;
            }
            else
            {
                mFlags &= ~FLAG_INUSE;
            }
        }

        AnimGraphPose& operator=(const AnimGraphPose& other);

    private:
        Pose    mPose;      /**< The pose, containing the node transformation. */
        uint8   mFlags;     /**< The flags. */
    };
}   // namespace EMotionFX
