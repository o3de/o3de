/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "EMotionFXConfig.h"
#include "AnimGraphEventBuffer.h"
#include "Transform.h"


namespace EMotionFX
{
    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphRefCountedData
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphRefCountedData, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA);

    public:
        MCORE_INLINE AnimGraphRefCountedData() = default;
        MCORE_INLINE ~AnimGraphRefCountedData() = default;

        MCORE_INLINE AnimGraphEventBuffer& GetEventBuffer()                     { return mEventBuffer; }
        MCORE_INLINE const AnimGraphEventBuffer& GetEventBuffer() const         { return mEventBuffer; }
        MCORE_INLINE void SetEventBuffer(const AnimGraphEventBuffer& buf)       { mEventBuffer = buf; }
        MCORE_INLINE void ClearEventBuffer()                                    { mEventBuffer.Clear(); }

        MCORE_INLINE Transform& GetTrajectoryDelta()                            { return mTrajectoryDelta; }
        MCORE_INLINE const Transform& GetTrajectoryDelta() const                { return mTrajectoryDelta; }
        MCORE_INLINE void SetTrajectoryDelta(const Transform& transform)        { mTrajectoryDelta = transform; }

        MCORE_INLINE Transform& GetTrajectoryDeltaMirrored()                    { return mTrajectoryDeltaMirrored; }
        MCORE_INLINE const Transform& GetTrajectoryDeltaMirrored() const        { return mTrajectoryDeltaMirrored; }
        MCORE_INLINE void SetTrajectoryDeltaMirrored(const Transform& tform)    { mTrajectoryDeltaMirrored = tform; }

        MCORE_INLINE void ZeroTrajectoryDelta()                                 { mTrajectoryDelta.IdentityWithZeroScale(); mTrajectoryDeltaMirrored.IdentityWithZeroScale(); }

    private:
        AnimGraphEventBuffer    mEventBuffer;
        Transform               mTrajectoryDelta = Transform::CreateIdentityWithZeroScale();
        Transform               mTrajectoryDeltaMirrored = Transform::CreateIdentityWithZeroScale();
    };
} // namespace EMotionFX
