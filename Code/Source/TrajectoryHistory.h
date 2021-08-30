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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Pose.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/tuple.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>


namespace EMotionFX
{
    namespace MotionMatching
    {
        class EMFX_API TrajectoryHistory
        {
        public:
            void Init(size_t jointIndex, size_t maxNumSamples, float numSecondsToTrack);
            void Clear();
            void Update(float timePassedInSeconds);
            void AddSample(const Pose& pose);
            void DebugDraw(ActorInstance* actorInstance, const AZ::Color& color);
            void DebugDrawSampled(ActorInstance* actorInstance, size_t numSamples, const AZ::Color& color);
            AZ::Vector3 SampleNormalized(float normalizedTime) const; // The normalizedTime parameter should be in range of 0 to 1.

            size_t GetNumSamples() const;
            size_t GetMaxNumSamples() const;
            float GetNumSecondsToTrack() const;
            float GetMinSampleTime() const;
            float GetMaxSampleTime() const;
            float GetCurrentTime() const;
            size_t GetJointIndex() const;

        private:
            size_t WrapIndex(size_t index) const;
            AZStd::tuple<size_t, size_t> FindSampleIndices(float sampleTime) const;

            AZStd::vector<float> m_times;
            AZStd::vector<AZ::Vector3> m_positions;
            size_t m_jointIndex = 0;
            size_t m_startIndex = 0;
            size_t m_endIndex = 0;
            size_t m_numSamples = 30;
            float m_currentTime = 0.0f;
            float m_numSecondsToTrack = 1.0f;
            float m_minSampleTime = 0.0f;
            float m_maxSampleTime = 0.0f;
        };
    } // namespace MotionMatching
} // namespace EMotionFX
