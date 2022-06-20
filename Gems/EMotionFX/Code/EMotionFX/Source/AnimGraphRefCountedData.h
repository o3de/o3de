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

        MCORE_INLINE AnimGraphEventBuffer& GetEventBuffer()                     { return m_eventBuffer; }
        MCORE_INLINE const AnimGraphEventBuffer& GetEventBuffer() const         { return m_eventBuffer; }
        MCORE_INLINE void SetEventBuffer(const AnimGraphEventBuffer& buf)       { m_eventBuffer = buf; }
        MCORE_INLINE void ClearEventBuffer()                                    { m_eventBuffer.Clear(); }

        MCORE_INLINE Transform& GetTrajectoryDelta()                            { return m_trajectoryDelta; }
        MCORE_INLINE const Transform& GetTrajectoryDelta() const                { return m_trajectoryDelta; }
        MCORE_INLINE void SetTrajectoryDelta(const Transform& transform)        { m_trajectoryDelta = transform; }

        MCORE_INLINE Transform& GetTrajectoryDeltaMirrored()                    { return m_trajectoryDeltaMirrored; }
        MCORE_INLINE const Transform& GetTrajectoryDeltaMirrored() const        { return m_trajectoryDeltaMirrored; }
        MCORE_INLINE void SetTrajectoryDeltaMirrored(const Transform& tform)    { m_trajectoryDeltaMirrored = tform; }

        MCORE_INLINE void ZeroTrajectoryDelta()                                 { m_trajectoryDelta.IdentityWithZeroScale(); m_trajectoryDeltaMirrored.IdentityWithZeroScale(); }

    private:
        AnimGraphEventBuffer    m_eventBuffer;
        Transform               m_trajectoryDelta = Transform::CreateIdentityWithZeroScale();
        Transform               m_trajectoryDeltaMirrored = Transform::CreateIdentityWithZeroScale();
    };
} // namespace EMotionFX
