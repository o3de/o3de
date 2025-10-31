/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Pose.h>

namespace EMotionFX
{
    class Motion;

    namespace MotionMatching
    {
        //! A motion matching frame.
        //! This holds information required in order to extract a given pose in a given motion.
        class EMFX_API Frame
        {
        public:
            AZ_RTTI(Frame, "{985BD732-D80E-4898-AB6C-CAB22D88AACD}")
            AZ_CLASS_ALLOCATOR_DECL

            Frame();
            Frame(size_t frameIndex, Motion* sourceMotion, float sampleTime, bool mirrored);
            ~Frame() = default;

            //! Sample the pose for the given frame.
            //! @param[in] outputPose The pose used to store the sampled result.
            //! @param[in] timeOffset Frames in the frame database are samples with a given sample rate (default = 30 fps).
            //!     For calculating velocities for example, it is needed to sample a pose close to a frame but not exactly at the frame position.
            //!     The timeOffset parameter can be used for that and represents the offset in time from the frame sample time in seconds.
            //!     In case the time offset is 0.0, the pose exactly at the frame position will be sampled.
            void SamplePose(Pose* outputPose, float timeOffset = 0.0f) const;

            Motion* GetSourceMotion() const;
            float GetSampleTime() const;
            size_t GetFrameIndex() const { return m_frameIndex; }
            bool GetMirrored() const { return m_mirrored; }

            void SetSourceMotion(Motion* sourceMotion);
            void SetSampleTime(float sampleTime);
            void SetFrameIndex(size_t frameIndex);
            void SetMirrored(bool enabled);

        private:
            size_t m_frameIndex = 0; //< The motion frame index inside the data object.
            float m_sampleTime = 0.0f; //< The time offset in the original motion.
            Motion* m_sourceMotion = nullptr; //< The original motion that we sample from to restore the pose.
            bool m_mirrored = false; //< Is this frame mirrored?
        };
    } // namespace MotionMatching
} // namespace EMotionFX
