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

#include <TrajectoryHistory.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <AzCore/std/algorithm.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        void TrajectoryHistory::Init(size_t jointIndex, size_t maxNumSamples, float numSecondsToTrack)
        {
            AZ_Assert(maxNumSamples > 0, "Max number of samples cannot be zero.");
            AZ_Assert(numSecondsToTrack > 0.0f, "Number of seconds to track has to be greater than zero.");
            Clear();
            m_times.resize(maxNumSamples);
            m_positions.resize(maxNumSamples);
            m_numSecondsToTrack = numSecondsToTrack;
            m_jointIndex = jointIndex;
        }

        void TrajectoryHistory::AddSample(const Pose& pose)
        {
            AZ_Assert(m_times.size() == m_positions.size(), "Expecting the size of m_times to be equal to m_positions.");

            // Check if the time passed since the last sample is smaller than the sample rate.
            // Because if so, let's not record this sample yet, as otherwise we get too many samples for the number of seconds we try to record.
            if (m_numSamples > 0)
            {
                const float timeSinceLastSample = m_currentTime - m_times[m_endIndex];
                const float spacingBetweenSamples = m_numSecondsToTrack / static_cast<float>(m_numSamples);
                AZ_Assert(timeSinceLastSample >= 0.0f, "Time since last sample has to be zero or larger.");
                if (timeSinceLastSample < spacingBetweenSamples)
                {
                    return;
                }
            }

            // Update the start and end indices by wrapping them if needed as we're a ring/circle buffer.
            m_numSamples++;
            if (m_numSamples < GetMaxNumSamples())
            {
                AZ_Assert(m_startIndex == 0, "Expected the start index to be zero.");
                m_endIndex++;
            }
            else
            {
                m_endIndex = WrapIndex(m_endIndex + 1);
                m_startIndex = WrapIndex(m_startIndex + 1);
                m_numSamples = GetMaxNumSamples();
                AZ_Assert(m_startIndex != m_endIndex, "Expected start and end index to not be the same.");
            }

            // Store the values.
            m_times[m_endIndex] = m_currentTime;
            m_positions[m_endIndex] = pose.GetWorldSpaceTransform(m_jointIndex).m_position;

            //AZ_Assert(GetMinSampleTime() <= GetMaxSampleTime(), "Expected the minimum sample time to be smaller than the maximum sample time.");
        }

        void TrajectoryHistory::Clear()
        {
            m_jointIndex = 0;
            m_startIndex = 0;
            m_endIndex = 0;
            m_numSamples = 0;
            m_minSampleTime = 0.0f;
            m_maxSampleTime = 0.0f;
            m_currentTime = 0.0f;
        }

        void TrajectoryHistory::Update(float timePassedInSeconds)
        {
            m_currentTime += timePassedInSeconds;
        }

        AZStd::tuple<size_t, size_t> TrajectoryHistory::FindSampleIndices(float sampleTime) const
        {
            AZ_Assert(m_numSamples > 1, "Expecting more than one sample.");

            if (sampleTime <= m_times[m_startIndex])
            {
                return { m_startIndex, m_startIndex };
            }

            if (sampleTime > m_times[m_endIndex])
            {
                return { m_endIndex, m_endIndex };
            }

            for (size_t i = 0; i < m_numSamples - 1; ++i)
            {
                const size_t firstIndex = WrapIndex(m_startIndex + i);
                const size_t secondIndex = WrapIndex(m_startIndex + i + 1);
                if (m_times[firstIndex] <= sampleTime &&
                    m_times[secondIndex] > sampleTime)
                {
                    return { firstIndex, secondIndex };
                }
            }

            return { m_endIndex, m_endIndex };
        }

        AZ::Vector3 TrajectoryHistory::SampleNormalized(float normalizedTime) const
        {
            //AZ_Assert(normalizedTime >= 0.0f && normalizedTime <= 1.0f, "Sample time has to be in range of 0 to 1.");
            if (m_numSamples == 0)
            {
                return AZ::Vector3::CreateZero();
            }

            if (m_numSamples == 1)
            {
                return m_positions[m_endIndex];
            }

            // Calculate the sample time.
            const float maxTime = GetMaxSampleTime();
            const float minTime = maxTime - m_numSecondsToTrack;
            const float sampleTime = maxTime - (normalizedTime * m_numSecondsToTrack);
            const float sampleTimeClamped = AZ::GetClamp(sampleTime, minTime, maxTime);

            // Find the two keys to interpoalte between.
            size_t firstSampleIndex;
            size_t secondSampleIndex;
            std::tie(firstSampleIndex, secondSampleIndex) = FindSampleIndices(sampleTimeClamped);
            const float firstSampleTime = m_times[firstSampleIndex];
            const float secondSampleTime = m_times[secondSampleIndex];

            // Interpolate betwen the two samples.
            const float spacingBetweenSamples = (secondSampleTime - firstSampleTime);
            if (spacingBetweenSamples > 0.0f)
            {
                const float alpha = (sampleTimeClamped - firstSampleTime) / spacingBetweenSamples;
                return m_positions[firstSampleIndex].Lerp(m_positions[secondSampleIndex], alpha);
            }
            else
            {
                return m_positions[firstSampleIndex];
            }
        }

        void TrajectoryHistory::DebugDraw(ActorInstance* actorInstance, const AZ::Color& color)
        {
            if (m_numSamples <= 1)
            {
                return;
            }

            // Start drawing.
            EMotionFX::DebugDraw& drawSystem = GetDebugDraw();
            drawSystem.Lock();
            EMotionFX::DebugDraw::ActorInstanceData* draw = drawSystem.GetActorInstanceData(actorInstance);
            draw->Lock();

            for (size_t i = 0; i < m_numSamples - 1; ++i)
            {
                const size_t sampleIndex = WrapIndex(m_startIndex + i);
                const size_t nextSampleIndex = WrapIndex(m_startIndex + i + 1);
                draw->DrawLine(m_positions[sampleIndex], m_positions[nextSampleIndex], color);
            }

            for (size_t i = 0; i < m_numSamples; ++i)
            {
                const size_t sampleIndex = WrapIndex(m_startIndex + i);
                draw->DrawMarker(m_positions[sampleIndex], AZ::Colors::White, 0.015f);
            }

            // End drawing.
            draw->Unlock();
            drawSystem.Unlock();
        }

        void TrajectoryHistory::DebugDrawSampled(ActorInstance* actorInstance, size_t numSamples, const AZ::Color& color)
        {
            if (m_numSamples <= 1)
            {
                return;
            }

            // Start drawing.
            EMotionFX::DebugDraw& drawSystem = GetDebugDraw();
            drawSystem.Lock();
            EMotionFX::DebugDraw::ActorInstanceData* draw = drawSystem.GetActorInstanceData(actorInstance);
            draw->Lock();

            // Draw the samples.
            numSamples = AZ::GetMin(numSamples, m_numSamples);
            AZ::Vector3 lastPos;
            for (size_t i = 0; i < numSamples; ++i)
            {
                const float sampleTime = i / static_cast<float>(numSamples - 1);
                const AZ::Vector3 currentPos = SampleNormalized(sampleTime);
                if (i > 0)
                {
                    draw->DrawLine(lastPos, currentPos, color);
                }

                draw->DrawMarker(currentPos, AZ::Colors::Yellow, 0.02f);
                lastPos = currentPos;
            }

            // End drawing.
            draw->Unlock();
            drawSystem.Unlock();
        }

        size_t TrajectoryHistory::GetNumSamples() const
        {
            return m_numSamples;
        }

        size_t TrajectoryHistory::GetMaxNumSamples() const
        {
            return m_times.size();
        }

        float TrajectoryHistory::GetNumSecondsToTrack() const
        {
            return m_numSecondsToTrack;
        }

        float TrajectoryHistory::GetMinSampleTime() const
        {
            return m_times[m_startIndex];
        }

        float TrajectoryHistory::GetMaxSampleTime() const
        {
            return m_times[m_endIndex];
        }

        size_t TrajectoryHistory::WrapIndex(size_t index) const
        {
            return index % GetMaxNumSamples();
        }

        float TrajectoryHistory::GetCurrentTime() const
        {
            return m_currentTime;
        }

        size_t TrajectoryHistory::GetJointIndex() const
        {
            return m_jointIndex;
        }
    } // namespace MotionMatching
} // namespace EMotionFX
