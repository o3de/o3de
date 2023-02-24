/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Allocators.h>
#include <Frame.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/TransformData.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(Frame, MotionMatchAllocator)

    Frame::Frame()
        : m_frameIndex(InvalidIndex)
        , m_sampleTime(0.0f)
        , m_sourceMotion(nullptr)
        , m_mirrored(false)
    {
    }

    Frame::Frame(size_t frameIndex, Motion* sourceMotion, float sampleTime, bool mirrored)
        : m_frameIndex(frameIndex)
        , m_sourceMotion(sourceMotion)
        , m_sampleTime(sampleTime)
        , m_mirrored(mirrored)
    {
    }

    void Frame::SamplePose(Pose* outputPose, float timeOffset) const
    {
        MotionDataSampleSettings sampleSettings;
        sampleSettings.m_actorInstance = outputPose->GetActorInstance();
        sampleSettings.m_inPlace = false;
        sampleSettings.m_mirror = m_mirrored;
        sampleSettings.m_retarget = false;
        sampleSettings.m_inputPose = sampleSettings.m_actorInstance->GetTransformData()->GetBindPose();

        sampleSettings.m_sampleTime = m_sampleTime + timeOffset;
        sampleSettings.m_sampleTime = AZ::GetClamp(m_sampleTime + timeOffset, 0.0f, m_sourceMotion->GetDuration());

        m_sourceMotion->SamplePose(outputPose, sampleSettings);
    }

    void Frame::SetFrameIndex(size_t frameIndex)
    {
        m_frameIndex = frameIndex;
    }

    Motion* Frame::GetSourceMotion() const
    {
        return m_sourceMotion;
    }

    float Frame::GetSampleTime() const
    {
        return m_sampleTime;
    }

    void Frame::SetSourceMotion(Motion* sourceMotion)
    {
        m_sourceMotion = sourceMotion;
    }

    void Frame::SetMirrored(bool enabled)
    {
        m_mirrored = enabled;
    }

    void Frame::SetSampleTime(float sampleTime)
    {
        m_sampleTime = sampleTime;
    }
} // namespace EMotionFX::MotionMatching
