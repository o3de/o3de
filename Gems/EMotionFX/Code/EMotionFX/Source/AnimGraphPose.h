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

        MCORE_INLINE size_t GetNumNodes() const                 { return m_pose.GetNumTransforms(); }
        MCORE_INLINE const Pose& GetPose() const                { return m_pose; }
        MCORE_INLINE Pose& GetPose()                            { return m_pose; }
        MCORE_INLINE void SetPose(const Pose& pose)             { m_pose = pose; }
        MCORE_INLINE const ActorInstance* GetActorInstance() const    { return m_pose.GetActorInstance(); }

        MCORE_INLINE bool GetIsInUse() const                    { return (m_flags & FLAG_INUSE); }
        MCORE_INLINE void SetIsInUse(bool inUse)
        {
            if (inUse)
            {
                m_flags |= FLAG_INUSE;
            }
            else
            {
                m_flags &= ~FLAG_INUSE;
            }
        }

        AnimGraphPose& operator=(const AnimGraphPose& other);

    private:
        Pose    m_pose;      /**< The pose, containing the node transformation. */
        uint8   m_flags;     /**< The flags. */
    };
}   // namespace EMotionFX
