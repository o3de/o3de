/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
