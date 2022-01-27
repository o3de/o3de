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

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <EMotionFX/Source/EMotionFXConfig.h>

namespace EMotionFX
{
    class Motion;

    namespace MotionMatching
    {
        /**
         * A motion matching frame.
         * This holds information required in order to extract a given pose in a given motion.
         */
        class EMFX_API Frame
        {
        public:
            AZ_RTTI(Frame, "{985BD732-D80E-4898-AB6C-CAB22D88AACD}")
            AZ_CLASS_ALLOCATOR_DECL

            Frame();
            Frame(size_t frameIndex, Motion* sourceMotion, float sampleTime, bool mirrored);
            ~Frame() = default;

            Motion* GetSourceMotion() const;
            float GetSampleTime() const;
            size_t GetFrameIndex() const { return m_frameIndex; }
            bool GetMirrored() const { return m_mirrored; }

            void SetSourceMotion(Motion* sourceMotion);
            void SetSampleTime(float sampleTime);
            void SetFrameIndex(size_t frameIndex);
            void SetMirrored(bool enabled);

        private:
            size_t m_frameIndex = 0; /**< The motion frame index inside the data object. */
            float m_sampleTime = 0.0f; /**< The time offset in the original motion. */
            Motion* m_sourceMotion = nullptr; /**< The original motion that we sample from to restore the pose. */
            bool m_mirrored = false; /**< Is this frame mirrored? */
        };
    } // namespace MotionMatching
} // namespace EMotionFX
