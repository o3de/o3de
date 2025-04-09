/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/RagdollVelocityEvaluators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(RagdollVelocityEvaluator, EMotionFX::ActorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(MovingAverageVelocityEvaluator, EMotionFX::ActorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(RunningAverageVelocityEvaluator, EMotionFX::ActorAllocator)

    void RagdollVelocityEvaluator::CalculateVelocities(Physics::RagdollState& outRagdollPose, const Physics::RagdollState& lastRagdollPose, const Physics::RagdollState& currentRagdollPose, float timeDelta)
    {
        const size_t numRagdollNodes = outRagdollPose.size();
        for (size_t i = 0; i < numRagdollNodes; ++i)
        {
            Physics::RagdollNodeState& nodePose = outRagdollPose[i];
            const Physics::RagdollNodeState& lastNodePose = lastRagdollPose[i];
            const Physics::RagdollNodeState& currentNodePose = currentRagdollPose[i];

            // Linear velocity
            nodePose.m_linearVelocity = (currentNodePose.m_position - lastNodePose.m_position) / timeDelta;

            // Angular velocity
            const AZ::Quaternion deltaRotation = currentNodePose.m_orientation * lastNodePose.m_orientation.GetConjugate();
            const float deltaRotX = static_cast<float>(deltaRotation.GetX());
            const float deltaRotY = static_cast<float>(deltaRotation.GetY());
            const float deltaRotZ = static_cast<float>(deltaRotation.GetZ());
            const float deltaRotW = static_cast<float>(deltaRotation.GetW());

            // Convert delta quaternion to axis angle representation and calculate angular velocity from that.
            const float length = sqrtf(1.0f - deltaRotW * deltaRotW);
            if (length > AZ::Constants::FloatEpsilon)
            {
                // The angle returned by GetAngle() is in range [0, twoPi]. Convert to the nearest angle in range [-pi, pi].
                const float angleCircleRange = static_cast<float>(deltaRotation.GetAngle());
                const float angle = angleCircleRange > AZ::Constants::Pi ? angleCircleRange - AZ::Constants::TwoPi : angleCircleRange;

                const AZ::Vector3 axis(deltaRotX/length, deltaRotY/length, deltaRotZ/length);

                nodePose.m_angularVelocity = axis * (angle / timeDelta);
            }
            else
            {
                nodePose.m_angularVelocity = AZ::Vector3::CreateZero();
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    MovingAverageVelocityEvaluator::MovingAverageVelocityEvaluator()
        : RagdollVelocityEvaluator()
        , m_minTimeWindow(0.1f)
    {
        m_poseHistory.resize(static_cast<size_t>(m_minTimeWindow*60.0f)); // Make sure there is enough space in the ring buffer for the time window at a desired framerate of 60.
    }

    void MovingAverageVelocityEvaluator::Update(const Physics::RagdollState& lastRagdollPose, const Physics::RagdollState& currentRagdollPose, float timeDelta)
    {
        // Do we need more space in the ring buffer for the historic poses? This might happen when the framerate accelerates after starting the game.
        if (CalcTotalHistoryTimeInterval() < m_minTimeWindow && m_poseHistory.capacity() == m_poseHistory.size())
        {
            // Grow by 50% similar to AZStd::vector's behavior.
            const size_t capacity = m_poseHistory.capacity();
            m_poseHistory.set_capacity(capacity + capacity / 2);
        }

        // Update the oldest pose in history pose ring buffer.
        AZStd::pair<Physics::RagdollState, float> pair = AZStd::make_pair(lastRagdollPose, timeDelta);
        CalculateVelocities(pair.first, lastRagdollPose, currentRagdollPose, timeDelta);
        m_poseHistory.push_back(pair);
    }

    void MovingAverageVelocityEvaluator::CalculateInitialVelocities(Physics::RagdollState& outRagdollPose)
    {
        // Early out and zero the initial velocities in case there are no historic poses.
        if (m_poseHistory.empty())
        {
            for (Physics::RagdollNodeState& nodePose : outRagdollPose)
            {
                nodePose.m_linearVelocity = AZ::Vector3::CreateZero();
                nodePose.m_angularVelocity = AZ::Vector3::CreateZero();
            }
            return;
        }

        // There might be more poses in the history buffer than we need to reach the minimum window time that we require
        // for calculating the initial velocities. This happens as the framerate isn't fully stable.
        // Accumulate the time deltas from the historic poses until we reached our minimum window time and get the index for the oldest pose.
        size_t oldestPoseInWindowIndex;
        float windowTime;
        CalcHistoryPoseIndex(oldestPoseInWindowIndex, windowTime);
        if (windowTime < AZ::Constants::FloatEpsilon)
        {
            return;
        }

        const AZ::s32 startPoseIndex = static_cast<AZ::s32>(m_poseHistory.size()) - 1;
        const AZ::s32 endPoseIndex = static_cast<AZ::s32>(oldestPoseInWindowIndex);

        const size_t numRagdollNodes = outRagdollPose.size();
        for (size_t n = 0; n < numRagdollNodes; ++n)
        {
            Physics::RagdollNodeState& nodePose = outRagdollPose[n];
            nodePose.m_linearVelocity = AZ::Vector3::CreateZero();
            nodePose.m_angularVelocity = AZ::Vector3::CreateZero();

            // Iterate from the youngest pose in history (the last pose) to more far away in time (front poses with lower indices).
            // Accumulate the linear and angular velocities based on their weight in the time window for the moving average.
            for (AZ::s32 h = startPoseIndex; h >= endPoseIndex; h--)
            {
                const float historyTimeDelta = m_poseHistory[h].second;
                const Physics::RagdollNodeState& historyNodePose = m_poseHistory[h].first[n];

                const float timeFractionInWindow = historyTimeDelta / windowTime;
                nodePose.m_linearVelocity += historyNodePose.m_linearVelocity * timeFractionInWindow;
                nodePose.m_angularVelocity += historyNodePose.m_angularVelocity * timeFractionInWindow;
            }
        }
    }

    float MovingAverageVelocityEvaluator::CalcTotalHistoryTimeInterval() const
    {
        float result = 0.0f;
        for (const auto& historyPair : m_poseHistory)
        {
            const float timeDelta = historyPair.second;
            result += timeDelta;
        }

        return result;
    }

    void MovingAverageVelocityEvaluator::CalcHistoryPoseIndex(size_t& outIndex, float& outTimeDelta)
    {
        outTimeDelta = 0.0f;
        const size_t numHistoryPoses = m_poseHistory.size();
        AZ_Assert(numHistoryPoses > 0, "Assuming the history is not empty.");
        outIndex = numHistoryPoses - 1;

        // Iterate from the youngest pose in history (the last pose) to more far away in time (front poses with lower indices).
        // Accumulate the time deltas until we reached our minimum required time interval and return.
        for (size_t i = 0; i < numHistoryPoses; ++i)
        {
            const size_t index = numHistoryPoses - 1 - i;
            outTimeDelta += m_poseHistory[index].second;
            outIndex = index;

            if (outTimeDelta > m_minTimeWindow)
            {
                return;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void RunningAverageVelocityEvaluator::Update(const Physics::RagdollState& lastRagdollPose, const Physics::RagdollState& currentRagdollPose, float timeDelta)
    {
        if (m_running.empty())
        {
            m_running = currentRagdollPose;
            for (Physics::RagdollNodeState& nodePose : m_running)
            {
                nodePose.m_linearVelocity = AZ::Vector3::CreateZero();
                nodePose.m_angularVelocity = AZ::Vector3::CreateZero();
            }
            m_last = m_running;
        }

        CalculateVelocities(m_last, lastRagdollPose, currentRagdollPose, timeDelta);

        const size_t numRagdollNodes = m_running.size();
        for (size_t i = 0; i < numRagdollNodes; ++i)
        {
            Physics::RagdollNodeState& runningNodePose = m_running[i];
            Physics::RagdollNodeState& lastNodePose = m_last[i];
            runningNodePose.m_linearVelocity = (runningNodePose.m_linearVelocity + lastNodePose.m_linearVelocity) * 0.5f;
            runningNodePose.m_angularVelocity = (runningNodePose.m_angularVelocity + lastNodePose.m_angularVelocity) * 0.5f;
        }
    }

    void RunningAverageVelocityEvaluator::CalculateInitialVelocities(Physics::RagdollState& outRagdollPose)
    {
        const size_t numRagdollNodes = outRagdollPose.size();
        if (numRagdollNodes == m_running.size())
        {
            for (size_t i = 0; i < numRagdollNodes; ++i)
            {
                outRagdollPose[i].m_linearVelocity = m_running[i].m_linearVelocity;
                outRagdollPose[i].m_angularVelocity = m_running[i].m_angularVelocity;
            }
        }
        else
        {
            // In case we are trying to calculate the initial velocities without having updated the evaluator once, zero-out the velocities.
            for (size_t i = 0; i < numRagdollNodes; ++i)
            {
                outRagdollPose[i].m_linearVelocity = AZ::Vector3::CreateZero();
                outRagdollPose[i].m_angularVelocity = AZ::Vector3::CreateZero();
            }
        }
    }
} // namespace EMotionFX
